#pragma once

#include <chrono>
#include <memory>
#include "rate_limiter/logger/Logger.hpp"
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

    bool handleRequest(const int& cost, const double& rate) {
        refillTokens(rate);
        if (tokens >= cost) {
            useTokens(cost);
            updateLastUsed();
            LOG_DEBUG("Request allowed: cost=", cost, " remaining_tokens=", tokens);
            return true;
        }
        LOG_DEBUG("Request denied: cost=", cost, " available_tokens=", tokens);
        return false;
    }

    bool hasExpired(const std::chrono::steady_clock::duration& ttl) {
        return ttl < Seconds(time_source->now() - lastused);
    }
};