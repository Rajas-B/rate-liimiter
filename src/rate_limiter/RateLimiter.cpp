#include "rate_limiter/RateLimiter.hpp"
#include "rate_limiter/time/SystemTimeSource.hpp"
#include "rate_limiter/logger/Logger.hpp"
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
    double calculateRate(const std::string& per, const int& rate) {
        if (per == "second") {
            return static_cast<double>(rate);
        }
        else if (per == "minute") {
            return static_cast<double>(rate) / 60.0;
        }
        else if (per == "hour") {
            return static_cast<double>(rate) / 3600.0;
        }
        else if (per == "day") {
            return static_cast<double>(rate) / 86400.0;
        }
        return -1.0;
    }

    std::string readFileContents(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            LOG_ERROR("Could not open file: ", path);
            return {};
        }
        return std::string(
            std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()
        );
    }
}

RateLimiter::RateLimiter()
    : RateLimiter(std::make_shared<SystemTimeSource>(), nullptr) {}

RateLimiter::RateLimiter(std::shared_ptr<TimeSource> time_source)
    : RateLimiter(std::move(time_source), nullptr) {}

RateLimiter::RateLimiter(std::shared_ptr<TimeSource> time_source,
                         std::shared_ptr<MetricsCollector> metrics)
    : time_source_(std::move(time_source)),
      metrics_(std::move(metrics)),
      endpoints_snapshot_(std::make_shared<const EndpointMap>()) {
    if (!metrics_) {
        metrics_ = std::make_shared<MetricsCollector>();
    }
}

RateLimiter::~RateLimiter() {
    stop();
}

void RateLimiter::start() {
    if (running_.exchange(true)) {
        LOG_INFO("RateLimiter already running");
        return;
    }
    LOG_INFO("RateLimiter started");
    worker_ = std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (running_) {
                LOG_DEBUG("Running background invalidation");
                invalidate();
            }
        }
        LOG_DEBUG("Background invalidation thread stopped");
    });
}

void RateLimiter::stop() {
    running_ = false;
    watching_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
    if (watcher_.joinable()) {
        watcher_.join();
    }
    LOG_INFO("RateLimiter stopped");
}

void RateLimiter::installEndpoints(std::shared_ptr<EndpointMap> new_endpoints) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    endpoints_snapshot_ = std::const_pointer_cast<const EndpointMap>(new_endpoints);
}

std::shared_ptr<const RateLimiter::EndpointMap> RateLimiter::loadEndpoints() const {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return endpoints_snapshot_;
}

std::string RateLimiter::normalizeMethod(const std::string& method) {
    std::string normalized;
    normalized.reserve(method.size());
    for (char c : method) {
        normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return normalized;
}

std::string RateLimiter::buildKey(const std::string& method, const std::string& path) {
    if (method.empty()) {
        return path;
    }
    return normalizeMethod(method) + " " + path;
}

std::shared_ptr<Endpoint> RateLimiter::findEndpoint(const std::string& key) const {
    auto endpoints = loadEndpoints();
    auto it = endpoints->find(key);
    if (it != endpoints->end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Endpoint> RateLimiter::resolveEndpoint(const std::string& method, const std::string& path) const {
    if (!method.empty()) {
        auto endpoint = findEndpoint(buildKey(method, path));
        if (endpoint) return endpoint;
    }
    auto endpoint = findEndpoint(path);
    if (endpoint) return endpoint;
    return findEndpoint("/*");
}

void RateLimiter::createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity) {
    createEndpoint("", endpoint, cost, rate, capacity);
}

void RateLimiter::createEndpoint(const std::string& method, const std::string& path,
                                 const int& cost, const double& rate, const int& capacity) {
    const std::string key = buildKey(method, path);
    auto endpoints = std::make_shared<EndpointMap>(*loadEndpoints());
    (*endpoints)[key] = std::make_shared<Endpoint>(key, rate, cost, capacity, time_source_, metrics_);
    installEndpoints(endpoints);
    LOG_INFO("Created endpoint: ", key, " cost=", cost, " rate=", rate, " capacity=", capacity);
}

bool RateLimiter::handleRequests(const std::string& endpoint, const std::string& uid) {
    auto ep = findEndpoint(endpoint);
    if (!ep) {
        ep = findEndpoint("/*");
    }
    if (!ep) {
        LOG_ERROR("Endpoint not found: ", endpoint);
        return false;
    }
    return ep->handleRequest(uid);
}

RateLimitResponse RateLimiter::checkLimit(const std::string& method, const std::string& path, const std::string& uid) {
    auto ep = resolveEndpoint(method, path);
    if (!ep) {
        RateLimitResponse response;
        response.allowed = true;
        response.limit = -1;
        response.remaining = -1;
        LOG_DEBUG("No endpoint matched for method=", method, " path=", path, " uid=", uid, " allowing request");
        return response;
    }
    return ep->checkLimit(uid);
}

void RateLimiter::parseConfigInto(const std::string& content, EndpointMap& target) {
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (!doc.IsArray()) {
        LOG_ERROR("JSON is not an array or is malformed.");
        return;
    }
    LOG_INFO("Parsing configuration");

    for (rapidjson::SizeType i = 0; i < doc.GetArray().Size(); ++i) {
        const rapidjson::Value& v = doc[i];

        if (!v.HasMember("endpoint") || !v.HasMember("capacity") ||
            !v.HasMember("cost") || !v.HasMember("per") || !v.HasMember("rate")) {
            continue;
        }

        std::string path = v["endpoint"].GetString();
        int cost = v["cost"].GetInt();
        int capacity = v["capacity"].GetInt();
        std::string per = v["per"].GetString();
        int rate_ = v["rate"].GetInt();

        std::string method;
        if (v.HasMember("method") && v["method"].IsString()) {
            method = v["method"].GetString();
        }

        double rate = calculateRate(per, rate_);
        if (rate >= 0.0) {
            const std::string key = buildKey(method, path);
            target[key] = std::make_shared<Endpoint>(key, rate, cost, capacity, time_source_, metrics_);
            LOG_INFO("Created endpoint: ", key, " cost=", cost, " rate=", rate, " capacity=", capacity);
        } else {
            LOG_ERROR("Invalid rate period for endpoint ", path, ": ", per);
        }
    }
    LOG_INFO("Configuration parsed: ", target.size(), " endpoint(s) created");
}

void RateLimiter::parseConfig(const std::string& content) {
    EndpointMap new_endpoints;
    parseConfigInto(content, new_endpoints);
    installEndpoints(std::make_shared<EndpointMap>(std::move(new_endpoints)));
    LOG_INFO("Configuration installed: ", loadEndpoints()->size(), " endpoint(s)");
}

void RateLimiter::readConfigFile(const std::string& path) {
    LOG_INFO("Reading config file: ", path);
    std::string content = readFileContents(path);
    if (!content.empty()) {
        parseConfig(content);
    }
}

void RateLimiter::reloadConfig(const std::string& path) {
    LOG_INFO("Reloading config file: ", path);
    std::string content = readFileContents(path);
    if (!content.empty()) {
        reloadConfigFromString(content);
    }
}

void RateLimiter::reloadConfigFromString(const std::string& content) {
    EndpointMap new_endpoints;
    parseConfigInto(content, new_endpoints);
    const std::size_t count = new_endpoints.size();
    installEndpoints(std::make_shared<EndpointMap>(std::move(new_endpoints)));
    LOG_INFO("Configuration reloaded: ", count, " endpoint(s)");
}

void RateLimiter::watchConfig(const std::string& path, std::chrono::milliseconds interval) {
    if (watching_.exchange(true)) {
        LOG_INFO("Config watcher already running");
        return;
    }
    watched_path_ = path;
    watch_interval_ = interval;
    if (std::filesystem::exists(path)) {
        last_file_write_ = std::filesystem::last_write_time(path);
    } else {
        last_file_write_ = std::filesystem::file_time_type::min();
    }
    watcher_ = std::thread([this]() { watcherLoop(); });
    LOG_INFO("Config watcher started for: ", path);
}

void RateLimiter::unwatchConfig() {
    watching_ = false;
    if (watcher_.joinable()) {
        watcher_.join();
    }
    LOG_INFO("Config watcher stopped");
}

void RateLimiter::watcherLoop() {
    while (watching_) {
        std::this_thread::sleep_for(watch_interval_);
        if (!watching_) {
            break;
        }
        try {
            if (!std::filesystem::exists(watched_path_)) {
                continue;
            }
            const auto modified = std::filesystem::last_write_time(watched_path_);
            if (modified > last_file_write_) {
                last_file_write_ = modified;
                LOG_INFO("Config file changed, reloading: ", watched_path_);
                reloadConfig(watched_path_);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR("Config watcher error: ", e.what());
        }
    }
}

void RateLimiter::invalidate() {
    auto endpoints = loadEndpoints();
    for (const auto& entry : *endpoints) {
        const auto& endpoint = entry.second;
        std::size_t removed = 0;
        for (auto& shard : endpoint->accessShards()) {
            std::lock_guard lock(shard.mutex);
            for (auto it = shard.buckets.begin(); it != shard.buckets.end(); ) {
                if (it->second.hasExpired(endpoint->accessTtl())) {
                    LOG_DEBUG("Expired bucket removed for uid=", it->first, " endpoint=", endpoint->key());
                    metrics_->onBucketEvicted(endpoint->key(), it->first);
                    it = shard.buckets.erase(it);
                    ++removed;
                } else {
                    ++it;
                }
            }
        }
        LOG_DEBUG("Invalidated ", removed, " expired bucket(s) for endpoint=", endpoint->key());
    }
}