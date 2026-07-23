#include "StressTest.hpp"
#include "rate_limiter/time/SystemTimeSource.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
    constexpr std::size_t kRequestsPerThread = 100'000;
    constexpr std::size_t kMinThroughput = 100'000;
}

StressTest::StressTest()
    : Test(std::make_shared<SystemTimeSource>()) {}

void StressTest::execute() {
    _rateLimiter.readConfigFile(TEST_RESOURCES_DIR"/stress_test_config.json");
    _rateLimiter.start();

    const unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        _rateLimiter.stop();
        assert(false && "Could not detect hardware concurrency");
    }

    std::atomic<std::size_t> allowed{0};
    std::atomic<std::size_t> denied{0};
    std::atomic<std::size_t> total{0};

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    const auto startTime = std::chrono::steady_clock::now();

    for (unsigned int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            const std::string uid = "stress-user-" + std::to_string(t);
            for (std::size_t i = 0; i < kRequestsPerThread; ++i) {
                const bool ok = _rateLimiter.handleRequests("/api/stress", uid);
                ++total;
                if (ok) {
                    ++allowed;
                } else {
                    ++denied;
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    const auto endTime = std::chrono::steady_clock::now();
    _rateLimiter.stop();

    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    const double elapsedSeconds = elapsedUs / 1'000'000.0;
    const std::size_t throughput = static_cast<std::size_t>(total.load() / elapsedSeconds);

    std::cout << "StressTest finished" << std::endl;
    std::cout << "  Threads:          " << threadCount << std::endl;
    std::cout << "  Total requests:   " << total.load() << std::endl;
    std::cout << "  Allowed:          " << allowed.load() << std::endl;
    std::cout << "  Denied:           " << denied.load() << std::endl;
    std::cout << "  Elapsed seconds:  " << elapsedSeconds << std::endl;
    std::cout << "  Throughput:       " << throughput << " req/s" << std::endl;

    assert(total.load() == threadCount * kRequestsPerThread);
    assert(allowed.load() + denied.load() == total.load());
    assert(throughput >= kMinThroughput);

    std::cout << "StressTest passed!" << std::endl;
}