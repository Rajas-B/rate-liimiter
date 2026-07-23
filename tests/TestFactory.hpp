#include "Test.hpp"
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

class TestFactory {
public:
    using FactoryFn = std::function<std::unique_ptr<Test>()>;
    std::unique_ptr<Test> getCurrentTest(const std::string& className);
    void getTests(const std::vector<std::string>& classNames, std::vector<std::unique_ptr<Test>>& result);
private:
    static const std::unordered_map<std::string, FactoryFn>& registry();
};