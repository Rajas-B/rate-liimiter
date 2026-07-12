#include <string>
#include <unordered_map>
#include "Endpoint.hpp"

class RateLimiter {
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Seconds = std::chrono::duration<double>;
public:
    void readConfigFile(const std::string& path);
    void createEndpoints();
    void createEndpoint(const std::string& endpoint, const int& cost, const double& rate, const int& capacity);
    void invalidate();

    bool handleRequests(const std::string& key, const std::string& endpoint);
private:
    std::unordered_map<std::string, Endpoint> endpoints;
};