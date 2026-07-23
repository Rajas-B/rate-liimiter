#include "MetricsTest.hpp"
#include <cassert>
#include <chrono>
#include <iostream>

MetricsTest::MetricsTest()
    : Test(std::make_shared<TestTimeSource>(), std::make_shared<CountingMetricsCollector>()) {
    _collector = std::dynamic_pointer_cast<CountingMetricsCollector>(_metrics);
}

void MetricsTest::execute() {
    _rateLimiter.createEndpoint("/api/metrics", 1, 1.0, 3);

    for (int i = 0; i < 3; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/metrics", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/metrics", "user1").allowed);
    assert(!_rateLimiter.checkLimit("GET", "/api/metrics", "user1").allowed);

    assert(_collector->allows.load() == 3);
    assert(_collector->denies.load() == 2);

    // Advance past TTL and run invalidation to trigger eviction metrics.
    auto testClock = testTimeSource();
    assert(testClock != nullptr);
    testClock->advance(std::chrono::seconds(10));
    _rateLimiter.invalidate();

    assert(_collector->evictions.load() == 1);

    std::cout << "MetricsTest passed!" << std::endl;
}
