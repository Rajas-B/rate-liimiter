#include "Endpoint.hpp"
#include <string>

bool Endpoint::handleRequest(const std::string& uid) {
    auto& bucketShard = bucketShards[std::hash<std::string>{} (uid) % shards];
    {
        std::lock_guard lock(bucketShard.mutex);
        auto& buckets = bucketShard.buckets;
        if (buckets.find(uid) == buckets.end())
            buckets.emplace(uid, Bucket(rate, capacity));
        return buckets[uid].handleRequest(cost, rate);
    }
    return false;
}

std::vector<BucketShard>& Endpoint::accessShards() {
    return bucketShards;
}

std::chrono::steady_clock::duration& Endpoint::accessTtl() {
    return ttl;
}