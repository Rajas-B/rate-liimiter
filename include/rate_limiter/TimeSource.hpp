#pragma once

#include <chrono>

class TimeSource {
public:
    virtual ~TimeSource() = default;
    virtual std::chrono::steady_clock::time_point now() const = 0;
};
