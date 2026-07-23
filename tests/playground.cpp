#include "rate_limiter/RateLimiter.hpp"
#include "rate_limiter/logger/Logger.hpp"
#include "Test.hpp"
#include "TestFactory.hpp"
#include <string>
#include <vector>

int main(int argc, char* argv[]) {

    Logger::instance().setLogFile("rate_limiter.log");
    LOG_INFO("Playground started");

    std::vector<std::string> testNames;
    testNames.reserve(argc-1);
    for (size_t i = 1; i < argc; ++i) {
        testNames.push_back(argv[i]);
    }
    TestFactory testFactory;    
    std::vector<std::unique_ptr<Test>> tests;

    testFactory.getTests(testNames, tests);

    for (auto& test: tests) {
        test->execute();
    }

    LOG_INFO("Playground finished");
    return 0;
}