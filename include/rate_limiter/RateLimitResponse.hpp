#pragma once

#include <chrono>

struct RateLimitResponse {
    bool allowed = false;
    int limit = -1;
    int remaining = -1;
    std::chrono::milliseconds retry_after{0};
    std::chrono::milliseconds reset_after{0};
};
