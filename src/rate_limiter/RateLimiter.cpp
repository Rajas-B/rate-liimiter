#include "rate_limiter/RateLimiter.hpp"
#include "rate_limiter/time/SystemTimeSource.hpp"
#include "rate_limiter/logger/Logger.hpp"
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <fstream>
#include <thread>

namespace {
    int calculateRate(const std::string& per, const int& rate) {
        if (per == "second") {
            return rate;
        }
        else if (per == "minute") {
            return rate/60;
        }
        else if (per == "hour") {
            return rate/3600;
        }
        else if (per == "day") {
            return rate/86400;
        }
        return -1;
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
    : RateLimiter(std::make_shared<SystemTimeSource>()) {}

RateLimiter::RateLimiter(std::shared_ptr<TimeSource> time_source)
    : time_source_(std::move(time_source)) {}

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
    if (worker_.joinable()) {
        worker_.join();
    }
    LOG_INFO("RateLimiter stopped");
}

void RateLimiter::createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity) {
    endpoints_.emplace(endpoint, Endpoint(endpoint, rate, cost, capacity, time_source_));
    LOG_INFO("Created endpoint: ", endpoint, " cost=", cost, " rate=", rate, " capacity=", capacity);
}

bool RateLimiter::handleRequests(const std::string& endpoint, const std::string& uid) {
    auto it = endpoints_.find(endpoint);
    if (it == endpoints_.end()) {
        LOG_ERROR("Endpoint not found: ", endpoint);
        return false;
    }
    return it->second.handleRequest(uid);
}

void RateLimiter::parseConfig(const std::string& content) {
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

        double rate = calculateRate(per, rate_);
        if (rate >= 0.0) {
            createEndpoint(path, cost, rate, capacity);
        } else {
            LOG_ERROR("Invalid rate period for endpoint ", path, ": ", per);
        }
    }
    LOG_INFO("Configuration parsed: ", endpoints_.size(), " endpoint(s) created");
}

void RateLimiter::readConfigFile(const std::string& path) {
    LOG_INFO("Reading config file: ", path);
    std::string content = readFileContents(path);
    if (!content.empty())
        parseConfig(content);
}

void RateLimiter::invalidate() {
    for (auto& endpoint : endpoints_) {
        std::size_t removed = 0;
        for (auto& shard : endpoint.second.accessShards()) {
            std::lock_guard lock(shard.mutex);
            for (auto it = shard.buckets.begin(); it != shard.buckets.end(); ) {
                if (it->second.hasExpired(endpoint.second.accessTtl())) {
                    LOG_DEBUG("Expired bucket removed for uid=", it->first, " endpoint=", endpoint.first);
                    it = shard.buckets.erase(it);
                    ++removed;
                } else {
                    ++it;
                }
            }
        }
        LOG_DEBUG("Invalidated ", removed, " expired bucket(s) for endpoint=", endpoint.first);
    }
}