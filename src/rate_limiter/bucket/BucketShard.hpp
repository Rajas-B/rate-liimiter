#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include "rate_limiter/bucket/Bucket.hpp"

struct BucketShard {
    std::unordered_map<std::string, Bucket> buckets;
    mutable std::mutex mutex;
};