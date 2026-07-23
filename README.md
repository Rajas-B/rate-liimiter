# Rate Limiter
## In C++

### Token bucket algorithm

A header-only-friendly C++17 token-bucket rate limiter designed to be embedded in a gateway or service mesh sidecar.

## Features

- **Token bucket rate limiting** per endpoint and per user.
- **Rich response metadata**: `allowed`, `limit`, `remaining`, `retry_after`, `reset_after`.
- **Per-method rules**: `GET /api/items` and `POST /api/items` can have different limits.
- **Default fallback rule**: use `"/*"` as a catch-all endpoint.
- **Hot reload**: reload limits from a file or string without restart; optional file watcher.
- **Observability**: pluggable `MetricsCollector` callback interface for allow/deny/evict events.
- **Thread-safe**: per-shard locking and RCU-style config updates.

## Configuration

The configuration is a JSON array of endpoint objects:

```json
[
    {
        "endpoint": "/api/items",
        "method": "GET",
        "rate": 100,
        "cost": 1,
        "per": "minute",
        "capacity": 100
    },
    {
        "endpoint": "/api/items",
        "method": "POST",
        "rate": 10,
        "cost": 1,
        "per": "minute",
        "capacity": 10
    },
    {
        "endpoint": "/*",
        "rate": 60,
        "cost": 1,
        "per": "minute",
        "capacity": 60
    }
]
```

- `endpoint` (required): the path. Use `"/*"` for the fallback rule.
- `method` (optional): HTTP method. When present, the rule key becomes `"METHOD /path"`.
- `rate`: number of tokens per `per` period.
- `cost`: tokens consumed per request.
- `per`: one of `second`, `minute`, `hour`, `day`.
- `capacity`: maximum bucket size.

## API

### Backward-compatible bool API

```cpp
RateLimiter limiter;
limiter.readConfigFile("resources/config.json");
bool allowed = limiter.handleRequests("/api/items", "user-123");
```

### Rich metadata API

```cpp
RateLimiter limiter;
limiter.readConfigFile("resources/config.json");
RateLimitResponse response = limiter.checkLimit("GET", "/api/items", "user-123");

// Map to HTTP headers:
// X-RateLimit-Limit:     response.limit
// X-RateLimit-Remaining: response.remaining
// X-RateLimit-Reset:     response.reset_after.count() / 1000
// Retry-After:           response.retry_after.count() / 1000
```

### Hot reload

```cpp
RateLimiter limiter;
limiter.readConfigFile("resources/config.json");

// Explicit reload
limiter.reloadConfig("resources/config.json");

// Or reload from a string
limiter.reloadConfigFromString(R"json([...])json");

// Optional background file watcher
limiter.watchConfig("resources/config.json", std::chrono::seconds(5));
```

### Metrics

Implement `MetricsCollector` and pass it to the constructor:

```cpp
class MyMetrics : public MetricsCollector {
public:
    void onDeny(const std::string& endpoint_key, const std::string& uid) override {
        // Increment Prometheus counter or alert on throttled client
    }
};

auto metrics = std::make_shared<MyMetrics>();
RateLimiter limiter(std::make_shared<SystemTimeSource>(), metrics);
```

The library only emits events; it does not depend on any metrics backend.

## Building

```bash
cmake -B build -S .
cmake --build build
```

Run tests:

```bash
./build/playground NormalTest
./build/playground MetadataTest
./build/playground PerMethodTest
./build/playground FallbackTest
./build/playground HotReloadTest
./build/playground MetricsTest
./build/playground BackgroundInvalidationTest
./build/playground StressTest
```

> **Note:** `StressTest` asserts a minimum throughput. Run it in Release mode for meaningful results because Debug builds enable verbose logging.

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/playground StressTest
```