#pragma once

#include "rate_limiter/TimeSource.hpp"

class SystemTimeSource : public TimeSource {
public:
    std::chrono::steady_clock::time_point now() const override {
        return std::chrono::steady_clock::now();
    }
};
