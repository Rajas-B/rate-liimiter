#include "Test.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include <atomic>
#include <string>

class CountingMetricsCollector : public MetricsCollector {
public:
    std::atomic<std::size_t> allows{0};
    std::atomic<std::size_t> denies{0};
    std::atomic<std::size_t> evictions{0};

    void onAllow(const std::string& endpoint_key, const std::string& uid) override {
        ++allows;
        (void)endpoint_key;
        (void)uid;
    }

    void onDeny(const std::string& endpoint_key, const std::string& uid) override {
        ++denies;
        (void)endpoint_key;
        (void)uid;
    }

    void onBucketEvicted(const std::string& endpoint_key, const std::string& uid) override {
        ++evictions;
        (void)endpoint_key;
        (void)uid;
    }
};

class MetricsTest : public Test {
public:
    MetricsTest();

private:
    std::shared_ptr<CountingMetricsCollector> _collector;
    void execute() override;
};

