#include "Test.hpp"
#include <string>

class TestFactory {
public:
    Test* getCurrentTest(const std::string& className);
};