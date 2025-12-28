#include "components/errorUtils.hpp"

#include <iostream>
#include <SDL3/SDL_log.h>
#include <stdio.h>
#include <stdarg.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

#if DEBUG
#include <cpptrace/cpptrace.hpp>
#endif

namespace vex {

    static std::string get_time_str() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    static const char* get_level_str(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARNING";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    static void log_internal(LogLevel level, const char* fmt, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);
        int len = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (len < 0) return;

        std::vector<char> buffer(len + 1);
        vsnprintf(buffer.data(), buffer.size(), fmt, args);

        std::string timeStr = get_time_str();
        const char* levelStr = get_level_str(level);

        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
                       "%s [%s] %s", timeStr.c_str(), levelStr, buffer.data());
    }

#if DEBUG
    [[noreturn]] void throw_error(const std::string& msg) {
        throw cpptrace::runtime_error(msg.c_str());
    }

    void log(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_internal(LogLevel::INFO, fmt, args);
        va_end(args);
    }

    void log(LogLevel level, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_internal(level, fmt, args);
        va_end(args);
    }

    void handle_exception(const std::exception& e) {
        if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
            cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
        } else {
            //SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
            log(LogLevel::CRITICAL, "%s", e.what());
        }
        log(LogLevel::ERROR, "Exception swallowed in DEBUG. Release build would crash here.");
    }

    void handle_critical_exception(const std::exception& e) {
        if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
            cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
        } else {
            log(LogLevel::CRITICAL, "%s", e.what());
        }
        throw;
    }

#else
    [[noreturn]] void throw_error(const std::string& msg) {
        throw std::runtime_error(msg.c_str());
    }

    void log(const char* fmt, ...) {
        // pass
    }

    void log(LogLevel level, const char* fmt, ...) {
        #ifndef DIST_BUILD
        if (level == LogLevel::INFO || level == LogLevel::WARNING) return;

        va_list args;
        va_start(args, fmt);
        log_internal(level, fmt, args);
        va_end(args);
        #endif
    }

    void handle_exception(const std::exception& e) {
        #ifndef DIST_BUILD
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        #endif
        throw;
    }

    void handle_critical_exception(const std::exception& e) {
        #ifndef DIST_BUILD
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        #endif
        throw;
    }

#endif

} // namespace vex
