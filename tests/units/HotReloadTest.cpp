#include "HotReloadTest.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

void HotReloadTest::execute() {
    // Initial config: capacity 2.
    _rateLimiter.reloadConfigFromString(R"json([
        {"endpoint": "/api/reload", "rate": 1, "cost": 1, "per": "second", "capacity": 2}
    ])json");

    for (int i = 0; i < 2; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/reload", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/reload", "user1").allowed);

    // Reload with a larger capacity.
    _rateLimiter.reloadConfigFromString(R"json([
        {"endpoint": "/api/reload", "rate": 1, "cost": 1, "per": "second", "capacity": 5}
    ])json");

    // New endpoint has fresh buckets, so 5 more requests are allowed.
    for (int i = 0; i < 5; ++i) {
        assert(_rateLimiter.checkLimit("GET", "/api/reload", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("GET", "/api/reload", "user1").allowed);

    // Reload with a per-method rule.
    _rateLimiter.reloadConfigFromString(R"json([
        {"endpoint": "/api/reload", "method": "POST", "rate": 1, "cost": 1, "per": "second", "capacity": 3}
    ])json");

    // GET no longer matches, so it should be allowed (no rule).
    auto response = _rateLimiter.checkLimit("GET", "/api/reload", "user1");
    assert(response.allowed);
    assert(response.limit == -1);

    // POST matches the new rule.
    for (int i = 0; i < 3; ++i) {
        assert(_rateLimiter.checkLimit("POST", "/api/reload", "user1").allowed);
    }
    assert(!_rateLimiter.checkLimit("POST", "/api/reload", "user1").allowed);

    std::cout << "HotReloadTest passed!" << std::endl;
}
