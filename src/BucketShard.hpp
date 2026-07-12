#include <unordered_map>
#include <string>
#include "Bucket.hpp"
#include <mutex>

struct BucketShard {
    std::unordered_map<std::string, Bucket> buckets;
    mutable std::mutex mutex;
};