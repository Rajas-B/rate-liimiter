#include "BackgroundInvalidationTest.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

void BackgroundInvalidationTest::execute() {
    // Endpoint: capacity=10, cost=1, rate=1.0 token/sec
    // TTL = 2 * capacity / rate = 20 seconds.
    _rateLimiter.createEndpoint("/api/invalidation-test", 1, 1.0, 10);

    const std::string uid = "user-invalidation";

    // Exhaust the bucket for this user.
    for (int i = 0; i < 10; ++i) {
        assert(_rateLimiter.handleRequests("/api/invalidation-test", uid));
    }
    assert(!_rateLimiter.handleRequests("/api/invalidation-test", uid));

    // Advance time past the bucket TTL so the background invalidator will remove it.
    auto testClock = testTimeSource();
    assert(testClock != nullptr);
    testClock->advance(std::chrono::seconds(25));

    // Start the background invalidation thread. It sleeps 1 second between runs.
    _rateLimiter.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    _rateLimiter.stop();

    // After invalidation the bucket should be gone, so a fresh bucket is created
    // with full capacity and the first 10 requests are allowed again.
    for (int i = 0; i < 10; ++i) {
        assert(_rateLimiter.handleRequests("/api/invalidation-test", uid));
    }
    assert(!_rateLimiter.handleRequests("/api/invalidation-test", uid));

    std::cout << "BackgroundInvalidationTest passed!" << std::endl;
}