#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>
#include "rate_limiter/bucket/Bucket.hpp"
#include "rate_limiter/bucket/BucketShard.hpp"
#include "rate_limiter/TimeSource.hpp"

class Endpoint {
public:
    Endpoint(std::string endpoint_, double rate_, int cost_, int capacity_,
             std::shared_ptr<TimeSource> time_source);
    ~Endpoint() = default;

    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;
    Endpoint(Endpoint&&) = default;
    Endpoint& operator=(Endpoint&&) = default;

    bool handleRequest(const std::string& uid);
    std::vector<BucketShard>& accessShards();
    std::chrono::steady_clock::duration& accessTtl();

private:
    int shards = 32;
    std::vector<BucketShard> bucketShards;
    std::string endpoint;
    double rate;
    int cost;
    int capacity;
    std::chrono::steady_clock::duration ttl;
    std::shared_ptr<TimeSource> time_source_;
};

