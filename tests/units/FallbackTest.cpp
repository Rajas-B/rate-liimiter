#include "FallbackTest.hpp"
#include <cassert>
#include <iostream>

void FallbackTest::execute() {
    // Specific rule for GET /api/specific.
    _rateLimiter.createEndpoint("GET", "/api/specific", 1, 1.0, 3);
    // Fallback rule for any other path/method.
    _rateLimiter.createEndpoint("/*", 1, 1.0, 5);

    // Specific rule applies.
    for (int i = 0; i < 3; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/specific", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/specific", "user1").allowed);

    // POST /api/specific has no specific rule, so it falls back to /*.
    for (int i = 0; i < 5; ++i) {
        assert(_rateLimiter.checkLimit("POST", "/api/specific", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("POST", "/api/specific", "user1").allowed);

    // Unknown path falls back to /*.
    for (int i = 0; i < 5; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/unknown", "user2").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/unknown", "user2").allowed);

    // Old bool API also uses fallback.
    assert(_rateLimiter.handleRequests("/api/unknown", "user3"));

    std::cout << "FallbackTest passed!" << std::endl;
}
