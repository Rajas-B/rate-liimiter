#include "NormalTest.hpp"
#include <cassert>
#include <chrono>
#include <iostream>

void NormalTest::execute() {
    _rateLimiter.createEndpoint("/api/test", 1, 1.0, 10);

    for (int i = 0; i < 10; ++i) {
        assert(_rateLimiter.handleRequests("/api/test", "user1"));
    }

    assert(!_rateLimiter.handleRequests("/api/test", "user1"));

    auto testClock = testTimeSource();
    assert(testClock != nullptr);
    testClock->advance(std::chrono::seconds(5));

    for (int i = 0; i < 5; ++i) {
        assert(_rateLimiter.handleRequests("/api/test", "user1"));
    }

    assert(!_rateLimiter.handleRequests("/api/test", "user1"));

    std::cout << "NormalTest passed!" << std::endl;
}