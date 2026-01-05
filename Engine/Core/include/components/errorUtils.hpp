/**
 * @file   errorUtils.hpp
 * @brief  This file defines functions for logging and throwing errors. It provides error tracing for debugging purposes that are not present for release builds.
 * @author Eryk Roszkowski
 ***********************************************/

 #pragma once
 #include <stdexcept>
 #include <string>

 #ifdef _WIN32
     #define WIN32_LEAN_AND_MEAN
     #define NOMINMAX
     #include <windows.h>

     #ifdef ERROR
         #undef ERROR
     #endif
 #endif

#include <vector>
 #include "VEX/VEX_export.h"

 namespace vex {

    /// @brief Enum class for log levels.
    enum class LogLevel {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    /// @brief Initializes the low-level crash handler.
    /// @details
    /// 1. Opens a persistent log file `game_session.log`.
    /// 2. **Windows**: Sets `SetUnhandledExceptionFilter`.
    /// 3. **Linux/Posix**: Sets signal handlers for `SIGSEGV`, `SIGABRT`, `SIGILL`, `SIGFPE`.
    /// The handler is capable of dumping CPU registers (RAX, RBX, etc.) and memory maps to the log file upon crash.
    void VEX_EXPORT InitCrashHandler();

    /// @brief Throws an error or terminates the application.
    /// @details
    /// - **Debug**: Throws a `cpptrace::runtime_error` to show a stack trace.
    /// - **Release**: Writes a crash report to the log file via `WriteCrashReport` and immediately exits with `_exit(1)`.
    /// @param const std::string& msg - The error message.
    /// @code
    ///     throw_error("An error occurred");
    /// @endcode
    [[noreturn]] void VEX_EXPORT throw_error(const std::string& msg);

    /// @brief Logs a formatted message.
    /// @details Internally calls `log_internal`, which:
    /// 1. Formats the string.
    /// 2. Pushes entry to a thread-safe ring buffer (`g_RingBuffer`).
    /// 3. Writes to the persistent crash log file (if open).
    /// 4. Prints to `SDL_LogMessage` (unless filtered by log level).
    /// @param const char* fmt - Printf-style format string.
    /// @code
    ///     log("value a is %d", a);
    /// @endcode
    void VEX_EXPORT log(const char* fmt, ...);

    /// @brief Logs a formatted message with a specific severity level.
    /// @param LogLevel level - Severity (INFO, WARNING, ERROR, CRITICAL).
    /// @param const char* fmt - Printf-style format string.
    /// @code
    ///     log(LogLevel::ERROR, "value a is %d", a);
    /// @endcode
    void VEX_EXPORT log(LogLevel level, const char* fmt, ...);

    /// @brief Handles an exception based on build configuration.
    /// @details
    /// - **Debug**: Prints a stack trace using `cpptrace`.
    /// - **Release**: Pushes the error to the ring buffer and logs via `SDL_LogCritical`. Does not crash the application.
    /// @param const std::exception& e - The exception to handle.
    /// @code
    /// try {
    ///     something();
    /// } catch (const std::exception& e) {
    ///     handle_exception(e);
    /// }
    /// @endcode
    void VEX_EXPORT handle_exception(const std::exception& e);

    /// @brief Handles a critical exception, forcing termination.
    /// @details
    /// - **Debug**: Prints stack trace and rethrows.
    /// - **Release**: Writes a full crash report, closes logs, calls `SDL_LogCritical`, and terminates via `_exit(1)`.
    /// @param const std::exception& e - The exception to handle.
    /// @code
    /// try {
    ///     something();
    /// } catch (const std::exception& e) {
    ///     handle_critical_exception(e);
    /// }
    /// @endcode
    void VEX_EXPORT handle_critical_exception(const std::exception& e);

 }
