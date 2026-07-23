#include "Test.hpp"
#include "NormalTest.hpp"
#include "BackgroundInvalidationTest.hpp"
#include "StressTest.hpp"
#include "MetadataTest.hpp"
#include "PerMethodTest.hpp"
#include "FallbackTest.hpp"
#include "HotReloadTest.hpp"
#include "MetricsTest.hpp"
#include "TestFactory.hpp"
#include <string>

const std::unordered_map<std::string, TestFactory::FactoryFn>& TestFactory::registry() {
    static const std::unordered_map<std::string, FactoryFn> map = {
        {"NormalTest", [] { return std::make_unique<NormalTest>(); }},
        {"BackgroundInvalidationTest", [] { return std::make_unique<BackgroundInvalidationTest>(); }},
        {"StressTest", [] { return std::make_unique<StressTest>(); }},
        {"MetadataTest", [] { return std::make_unique<MetadataTest>(); }},
        {"PerMethodTest", [] { return std::make_unique<PerMethodTest>(); }},
        {"FallbackTest", [] { return std::make_unique<FallbackTest>(); }},
        {"HotReloadTest", [] { return std::make_unique<HotReloadTest>(); }},
        {"MetricsTest", [] { return std::make_unique<MetricsTest>(); }},
    };
    return map;
}

std::unique_ptr<Test> TestFactory::getCurrentTest(const std::string& className) {
    const auto& map = registry();
    const auto it = map.find(className);

    if (it == map.end()) {
        throw std::invalid_argument("Unknown test class: " + className);
    }

    return it->second();
}

void TestFactory::getTests(const std::vector<std::string>& classNames, std::vector<std::unique_ptr<Test>>& result) {
    result.reserve(classNames.size());
    for (const auto& name: classNames) {
        result.push_back(getCurrentTest(name));
    }
}