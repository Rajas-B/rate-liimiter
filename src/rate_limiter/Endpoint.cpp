#include "rate_limiter/Endpoint.hpp"
#include "rate_limiter/logger/Logger.hpp"
#include <string>

Endpoint::Endpoint(std::string endpoint_, double rate_, int cost_, int capacity_,
                   std::shared_ptr<TimeSource> time_source)
    : endpoint(endpoint_),
      rate(rate_),
      cost(cost_),
      capacity(capacity_),
      bucketShards(shards),
      ttl(std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<double>(2.0 * static_cast<double>(capacity_) / rate_))),
      time_source_(std::move(time_source)) {}

bool Endpoint::handleRequest(const std::string& uid) {
    const std::size_t shardIndex = std::hash<std::string>{}(uid) % shards;
    LOG_DEBUG("Handling request for endpoint=", endpoint, " uid=", uid, " shard=", shardIndex);
    auto& bucketShard = bucketShards[shardIndex];
    {
        std::lock_guard lock(bucketShard.mutex);
        auto& buckets = bucketShard.buckets;
        auto it = buckets.find(uid);
        if (it == buckets.end()) {
            LOG_DEBUG("Creating new bucket for uid=", uid, " endpoint=", endpoint);
            it = buckets.emplace(uid, Bucket(capacity, capacity, time_source_)).first;
        }
        const bool allowed = it->second.handleRequest(cost, rate);
        LOG_DEBUG("Request ", allowed ? "allowed" : "denied", " for uid=", uid, " endpoint=", endpoint);
        return allowed;
    }
    return false;
}

std::vector<BucketShard>& Endpoint::accessShards() {
    return bucketShards;
}

std::chrono::steady_clock::duration& Endpoint::accessTtl() {
    return ttl;
}