#pragma once

#include "Test.hpp"

class StressTest : public Test {
public:
    StressTest();
    void execute() override;
};