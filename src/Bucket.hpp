#include <chrono>
struct Bucket {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Seconds = std::chrono::duration<double>;

    Bucket(int tokens, int capacity): 
        tokens(tokens), capacity(capacity), lastused(Clock::now()) {}

    TimePoint lastused;
    double tokens;
    int capacity;

    inline void useTokens(const int& cost) {
        tokens -= cost;
    }

    inline void updateLastUsed() {
        lastused = Clock::now();
    }

    void refillTokens(const double& rate) {
        auto gap = Seconds(Clock::now() - lastused).count();
        tokens += gap*rate;
        tokens = tokens>capacity?capacity:tokens;
    }

    bool handleRequest(const int& cost, const double& rate) {
        refillTokens(rate);
        if (tokens >= cost) {
            useTokens(cost);
            updateLastUsed();
            return true;
        }
        return false;
    }

    bool hasExpired (const Clock::duration& ttl) {
        return ttl < Seconds(Clock::now() - lastused);
    }

};