#include <string>
#include <unordered_map>
#include <vector>
#include "Bucket.hpp"
#include "BucketShard.hpp"
#include <chrono>

class Endpoint {
    
    public:
    Endpoint(std::string endpoint_, double rate_, int cost_, int capacity_): 
    endpoint(endpoint_), rate(rate_), cost(cost_), capacity(capacity_), 
    bucketShards(shards), 
    ttl(std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(2.0 * static_cast<double>(capacity_)/rate_)
    )){
    }
    ~Endpoint();
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
};

