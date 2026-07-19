#pragma once

#include <memory>
#include "rate_limiter/RateLimiter.hpp"
#include "utils/TestTimeSource.hpp"

class Test {
protected:
    std::shared_ptr<TimeSource> _timeSource;
    RateLimiter _rateLimiter;

    std::shared_ptr<TestTimeSource> testTimeSource() const {
        return std::dynamic_pointer_cast<TestTimeSource>(_timeSource);
    }

public:
    Test()
        : _timeSource(std::make_shared<TestTimeSource>()),
          _rateLimiter(_timeSource) {}

    explicit Test(std::shared_ptr<TimeSource> timeSource)
        : _timeSource(std::move(timeSource)),
          _rateLimiter(_timeSource) {}

    virtual ~Test() = default;
    virtual void execute() = 0;
};