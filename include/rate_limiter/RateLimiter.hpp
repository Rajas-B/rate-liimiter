#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include "rate_limiter/Endpoint.hpp"
#include "rate_limiter/TimeSource.hpp"

class RateLimiter {
public:
    RateLimiter();
    explicit RateLimiter(std::shared_ptr<TimeSource> time_source);
    ~RateLimiter();

    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;
    RateLimiter(RateLimiter&&) = delete;
    RateLimiter& operator=(RateLimiter&&) = delete;

    void start();
    void stop();

    void readConfigFile(const std::string& path);
    void createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity);
    void invalidate();
    void parseConfig(const std::string& content);
    bool handleRequests(const std::string& key, const std::string& endpoint);

private:
    std::shared_ptr<TimeSource> time_source_;
    std::unordered_map<std::string, Endpoint> endpoints_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};