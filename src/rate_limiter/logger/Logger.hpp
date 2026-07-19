#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel {
    INFO,
    ERROR,
    DEBUG
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(path, std::ios::out | std::ios::trunc);
    }

    template <typename... Args>
    void info(Args&&... args) {
        log(LogLevel::INFO, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(Args&&... args) {
        log(LogLevel::ERROR, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(Args&&... args) {
        log(LogLevel::DEBUG, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log(LogLevel level, Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        const std::string message = oss.str();

        std::lock_guard<std::mutex> lock(mutex_);

        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) % 1000;

        std::ostringstream line;
        line << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        line << '.' << std::setfill('0') << std::setw(3) << ms.count();
        line << " [" << levelToString(level) << "] ";
        line << message;

        if (level == LogLevel::ERROR) {
            std::cerr << line.str() << std::endl;
        } else {
            std::cout << line.str() << std::endl;
        }

        if (file_.is_open()) {
            file_ << line.str() << std::endl;
        }
    }

private:
    Logger() = default;
    ~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::INFO: return "INFO";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::DEBUG: return "DEBUG";
        }
        return "UNKNOWN";
    }

    std::mutex mutex_;
    std::ofstream file_;
};

#define LOG_INFO(...)  Logger::instance().info(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)

#ifdef LOG_DEBUG_ENABLED
    #define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#else
    #define LOG_DEBUG(...) (void)0
#endif
