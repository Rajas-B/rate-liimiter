#pragma once

#include "rate_limiter/TimeSource.hpp"
#include <chrono>

class TestTimeSource : public TimeSource {
public:
    TestTimeSource()
        : current_time_(std::chrono::steady_clock::time_point{}) {}

    std::chrono::steady_clock::time_point now() const override {
        return current_time_;
    }

    void advance(std::chrono::seconds s) {
        current_time_ += s;
    }

    void set(std::chrono::steady_clock::time_point tp) {
        current_time_ = tp;
    }

private:
    std::chrono::steady_clock::time_point current_time_;
};
