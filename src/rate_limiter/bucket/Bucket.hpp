#pragma once

#include <chrono>
#include <cmath>
#include <memory>
#include "rate_limiter/logger/Logger.hpp"
#include "rate_limiter/RateLimitResponse.hpp"
#include "rate_limiter/TimeSource.hpp"

struct Bucket {
    using TimePoint = std::chrono::steady_clock::time_point;
    using Seconds = std::chrono::duration<double>;

    Bucket(int tokens, int capacity, std::shared_ptr<TimeSource> time_source)
        : tokens(tokens),
          capacity(capacity),
          lastused(time_source->now()),
          time_source(std::move(time_source)) {}

    TimePoint lastused;
    double tokens;
    int capacity;
    std::shared_ptr<TimeSource> time_source;

    inline void useTokens(const int& cost) {
        tokens -= cost;
    }

    inline void updateLastUsed() {
        lastused = time_source->now();
    }

    void refillTokens(const double& rate) {
        auto gap = Seconds(time_source->now() - lastused).count();
        const double added = gap * rate;
        tokens += added;
        tokens = tokens > capacity ? capacity : tokens;
        LOG_DEBUG("Refilled tokens: +", added, " tokens=", tokens, " capacity=", capacity);
    }

    RateLimitResponse handleRequest(const int& cost, const double& rate) {
        refillTokens(rate);
        RateLimitResponse response;
        response.limit = capacity;
        response.remaining = static_cast<int>(std::floor(tokens));

        if (tokens >= cost) {
            useTokens(cost);
            updateLastUsed();
            response.allowed = true;
            response.remaining = static_cast<int>(std::floor(tokens));
            response.retry_after = std::chrono::milliseconds{0};
            response.reset_after = timeToFull(rate);
            LOG_DEBUG("Request allowed: cost=", cost, " remaining_tokens=", tokens);
            return response;
        }

        response.allowed = false;
        response.retry_after = timeUntilTokens(cost - tokens, rate);
        response.reset_after = timeToFull(rate);
        LOG_DEBUG("Request denied: cost=", cost, " available_tokens=", tokens);
        return response;
    }

    bool handleRequestBool(const int& cost, const double& rate) {
        return handleRequest(cost, rate).allowed;
    }

    bool hasExpired(const std::chrono::steady_clock::duration& ttl) {
        return ttl < Seconds(time_source->now() - lastused);
    }

private:
    std::chrono::milliseconds timeUntilTokens(double needed, double rate) const {
        if (rate <= 0.0 || needed <= 0.0) {
            return std::chrono::milliseconds{0};
        }
        const double seconds = needed / rate;
        return std::chrono::milliseconds{static_cast<std::int64_t>(std::ceil(seconds * 1000.0))};
    }

    std::chrono::milliseconds timeToFull(double rate) const {
        const double needed = static_cast<double>(capacity) - tokens;
        return timeUntilTokens(needed, rate);
    }
};