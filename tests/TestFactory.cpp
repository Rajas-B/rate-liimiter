#include "Test.hpp"
#include "NormalTest.hpp"
#include "BackgroundInvalidationTest.hpp"
#include "StressTest.hpp"
#include "TestFactory.hpp"
#include <string>

Test* TestFactory::getCurrentTest(const std::string& className) {
    if (className == "NormalTest") {
        return new NormalTest();
    }
    if (className == "BackgroundInvalidationTest") {
        return new BackgroundInvalidationTest();
    }
    if (className == "StressTest") {
        return new StressTest();
    }
    return new NormalTest();
}