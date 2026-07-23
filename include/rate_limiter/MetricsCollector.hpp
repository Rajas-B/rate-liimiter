#pragma once

#include <string>

class MetricsCollector {
public:
    virtual ~MetricsCollector() = default;

    virtual void onAllow(const std::string& endpoint_key, const std::string& uid) {
        (void)endpoint_key;
        (void)uid;
    }

    virtual void onDeny(const std::string& endpoint_key, const std::string& uid) {
        (void)endpoint_key;
        (void)uid;
    }

    virtual void onBucketEvicted(const std::string& endpoint_key, const std::string& uid) {
        (void)endpoint_key;
        (void)uid;
    }
};
