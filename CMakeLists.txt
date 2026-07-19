cmake_minimum_required(VERSION 3.14)
project(RateLimiter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(RapidJSON REQUIRED)

# ---- Library ----
add_library(rate_limiter STATIC
    src/rate_limiter/RateLimiter.cpp
    src/rate_limiter/Endpoint.cpp
)

# Public headers: exposed to consumers of the library
target_include_directories(rate_limiter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Private headers: only used while building the library
target_include_directories(rate_limiter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(rate_limiter
    PUBLIC Threads::Threads
)

target_include_directories(rate_limiter PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

target_compile_definitions(rate_limiter PUBLIC $<$<CONFIG:Debug>:LOG_DEBUG_ENABLED>)

# ---- Test runner (playground) ----
add_executable(playground
    tests/playground.cpp
    tests/NormalTest.cpp
    tests/BackgroundInvalidationTest.cpp
    tests/StressTest.cpp
    tests/TestFactory.cpp
)

target_include_directories(playground PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(playground PRIVATE rate_limiter)

# Copy test resources next to the executable so relative paths work at runtime
add_custom_command(TARGET playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/resources
    $<TARGET_FILE_DIR:playground>/resources
)