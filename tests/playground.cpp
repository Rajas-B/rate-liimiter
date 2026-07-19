#include "rate_limiter/RateLimiter.hpp"
#include "rate_limiter/logger/Logger.hpp"
#include "Test.hpp"
#include "TestFactory.hpp"
#include <string>

int main(int argc, char* argv[]) {
    Logger::instance().setLogFile("rate_limiter.log");
    LOG_INFO("Playground started");

    const std::string testName = (argc > 1) ? argv[1] : "NormalTest";
    LOG_INFO("Running test: ", testName);

    TestFactory testFactory;
    Test* test = testFactory.getCurrentTest(testName);
    test->execute();
    delete test;

    LOG_INFO("Playground finished");
    return 0;
}