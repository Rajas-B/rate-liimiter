#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include "rate_limiter/Endpoint.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/RateLimitResponse.hpp"
#include "rate_limiter/TimeSource.hpp"

class RateLimiter {
public:
    RateLimiter();
    explicit RateLimiter(std::shared_ptr<TimeSource> time_source);
    explicit RateLimiter(std::shared_ptr<TimeSource> time_source,
                         std::shared_ptr<MetricsCollector> metrics);
    ~RateLimiter();

    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;
    RateLimiter(RateLimiter&&) = delete;
    RateLimiter& operator=(RateLimiter&&) = delete;

    void start();
    void stop();

    void readConfigFile(const std::string& path);
    void parseConfig(const std::string& content);
    void reloadConfig(const std::string& path);
    void reloadConfigFromString(const std::string& content);

    void watchConfig(const std::string& path, std::chrono::milliseconds interval);
    void unwatchConfig();

    void createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity);
    void createEndpoint(const std::string& method, const std::string& path,
                        const int& cost, const double& rate, const int& capacity);

    bool handleRequests(const std::string& endpoint, const std::string& uid);
    RateLimitResponse checkLimit(const std::string& method, const std::string& path, const std::string& uid);

    void invalidate();

private:
    using EndpointMap = std::unordered_map<std::string, std::shared_ptr<Endpoint>>;

    std::shared_ptr<TimeSource> time_source_;
    std::shared_ptr<MetricsCollector> metrics_;
    std::shared_ptr<const EndpointMap> endpoints_snapshot_;
    mutable std::mutex endpoints_mutex_;
    std::atomic<bool> running_{false};
    std::atomic<bool> watching_{false};
    std::thread worker_;
    std::thread watcher_;
    std::string watched_path_;
    std::chrono::milliseconds watch_interval_{0};
    std::filesystem::file_time_type last_file_write_;

    void installEndpoints(std::shared_ptr<EndpointMap> new_endpoints);
    std::shared_ptr<const EndpointMap> loadEndpoints() const;
    std::shared_ptr<Endpoint> findEndpoint(const std::string& key) const;
    std::shared_ptr<Endpoint> resolveEndpoint(const std::string& method, const std::string& path) const;
    static std::string buildKey(const std::string& method, const std::string& path);
    static std::string normalizeMethod(const std::string& method);
    void parseConfigInto(const std::string& content, EndpointMap& target);
    void watcherLoop();
};