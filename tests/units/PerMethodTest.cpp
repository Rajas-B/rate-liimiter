#include "PerMethodTest.hpp"
#include <cassert>
#include <iostream>

void PerMethodTest::execute() {
    // GET /api/items: capacity 5, cost 1, rate 1.0
    _rateLimiter.createEndpoint("GET", "/api/items", 1, 1.0, 5);
    // POST /api/items: capacity 2, cost 1, rate 1.0
    _rateLimiter.createEndpoint("POST", "/api/items", 1, 1.0, 2);

    // Exhaust GET bucket.
    for (int i = 0; i < 5; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/items", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/items", "user1").allowed);

    // POST bucket should still have its own capacity.
    assert(_rateLimiter.checkLimit("POST", "/api/items", "user1").allowed);
    assert(_rateLimiter.checkLimit("POST", "/api/items", "user1").allowed);
    assert(!_rateLimiter.checkLimit("POST", "/api/items", "user1").allowed);

    // Method normalization: lowercase "get" should map to the same rule.
    auto testClock = testTimeSource();
    assert(testClock != nullptr);
    testClock->advance(std::chrono::seconds(5));
    assert(_rateLimiter.checkLimit("get", "/api/items", "user1").allowed);

    std::cout << "PerMethodTest passed!" << std::endl;
}
