#include "MetadataTest.hpp"
#include <cassert>
#include <chrono>
#include <iostream>

void MetadataTest::execute() {
    _rateLimiter.createEndpoint("/api/meta", 1, 1.0, 10);

    // Exhaust the bucket.
    for (int i = 0; i < 10; ++i) {
        auto response = _rateLimiter.checkLimit("GET", "/api/meta", "user1");
        assert(response.allowed);
        assert(response.limit == 10);
        assert(response.remaining == 10 - i - 1);
        assert(response.retry_after.count() == 0);
    }

    // Next request should be denied with metadata.
    auto denied = _rateLimiter.checkLimit("GET", "/api/meta", "user1");
    assert(!denied.allowed);
    assert(denied.limit == 10);
    assert(denied.remaining == 0);
    assert(denied.retry_after.count() > 0);
    assert(denied.reset_after.count() > 0);

    // Advance time to refill one token.
    auto testClock = testTimeSource();
    assert(testClock != nullptr);
    testClock->advance(std::chrono::seconds(1));

    auto allowed = _rateLimiter.checkLimit("GET", "/api/meta", "user1");
    assert(allowed.allowed);
    assert(allowed.remaining == 0); // 1 token consumed immediately
    assert(allowed.retry_after.count() == 0);

    std::cout << "MetadataTest passed!" << std::endl;
}
