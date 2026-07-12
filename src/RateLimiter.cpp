#include "RateLimiter.hpp" 
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <iostream>
#include <fstream>

namespace {
    int calculateRate(const std::string& per, const int& rate) {
        if (per == "second") {
            return rate;
        }
        else if (per == "minute") {
            return rate/60;
        }
        else if (per == "hour") {
            return rate/3600;
        }
        else if (per == "day") {
            return rate/86400;
        }
        return -1;
    }
}

void RateLimiter::createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity) {
    endpoints.emplace(endpoint, new Endpoint(endpoint, rate, cost, capacity));
}

bool RateLimiter::handleRequests(const std::string& endpoint, const std::string& uid) {
    return endpoints[endpoint].handleRequest(uid);
}


void RateLimiter::readConfigFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "Could not open file: " << path << std::endl;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), 
                        (std::istreambuf_iterator<char>()));

    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.IsArray()) {
        for (rapidjson::SizeType i = 0; i < doc.GetArray().Size(); i++) {
            const rapidjson::Value& v = doc[i];
            
            if (v.HasMember("endpoint") && v.HasMember("capacity") && v.HasMember("cost") && v.HasMember("per") && v.HasMember("rate")) {
                std::string path = v["endpoint"].GetString();
                int requests = v["requests"].GetInt();
                int cost = v["cost"].GetInt();
                int capacity = v["capacity"].GetInt();
                std::string per = v["per"].GetString();
                int rate_ = v["rate"].GetInt();
                double rate = calculateRate(per, rate_);
                if (rate != -1)
                    endpoints.emplace(path, Endpoint(path, rate, cost, capacity));
            }
        }
    } else {
        std::cerr << "JSON is not an array or is malformed." << std::endl;
    }
}

void RateLimiter::invalidate() {
    for (auto& endpoint: endpoints) {
        for (auto& shard: endpoint.second.accessShards()) {
            {
                std::lock_guard lock(shard.mutex);
                for (auto& bucket: shard.buckets){
                    // invalidate
                    if (bucket.second.hasExpired(endpoint.second.accessTtl())) 
                        shard.buckets.erase(bucket.first);
                }
            }
        }
    }
}