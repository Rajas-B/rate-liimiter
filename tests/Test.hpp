#pragma once

#include <memory>
#include "rate_limiter/RateLimiter.hpp"
#include "utils/TestTimeSource.hpp"

class Test {
protected:
    std::shared_ptr<TimeSource> _timeSource;
    std::shared_ptr<MetricsCollector> _metrics;
    RateLimiter _rateLimiter;

    std::shared_ptr<TestTimeSource> testTimeSource() const {
        return std::dynamic_pointer_cast<TestTimeSource>(_timeSource);
    }

public:
    Test()
        : _timeSource(std::make_shared<TestTimeSource>()),
          _metrics(nullptr),
          _rateLimiter(_timeSource) {}

    explicit Test(std::shared_ptr<TimeSource> timeSource)
        : _timeSource(std::move(timeSource)),
          _metrics(nullptr),
          _rateLimiter(_timeSource) {}

    Test(std::shared_ptr<TimeSource> timeSource,
         std::shared_ptr<MetricsCollector> metrics)
        : _timeSource(std::move(timeSource)),
          _metrics(std::move(metrics)),
          _rateLimiter(_timeSource, _metrics) {}

    virtual ~Test() = default;
    virtual void execute() = 0;
};