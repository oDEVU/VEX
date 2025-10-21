/**
 *  @file   errorUtils.hpp
 *  @brief  This file defines functions for logging and throwing errors. It provides error tracing for debugging purposes that are not present for release builds.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <stdexcept>
#include <iostream>
#include <SDL3/SDL_log.h>
#include <stdio.h>
#include <stdarg.h>

#if DEBUG
#include <cpptrace/cpptrace.hpp>

inline void throw_error(const std::string& msg) {
    throw cpptrace::runtime_error(msg.c_str());
}

inline void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
    va_end(args);
}

inline void handle_exception(const std::exception& e) {
    if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
        cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
    } else {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
    }
    throw;
}

#else
/// @brief Throws an error with the given message and triggers cpptrace for debug builds.
/// @details Example usage:
/// @code
///     throw_error("An error occurred");
/// @endcode
inline void throw_error(const std::string& msg) {
    throw std::runtime_error(msg.c_str());
}

/// @brief Logs a message with the given format and arguments to the console, or does absolutely nothing for release builds.
/// @details Example usage:
/// @code
///     log("value a is %d", a);
/// @endcode
inline void log(const char* fmt, ...) {
    return; // temponary
}

/// @brief Handles an exception by logging it and rethrowing it.
/// @details Example usage:
/// @code
/// try {
///     something();
/// } catch (const std::exception& e) {
///     handle_exception(e);
/// }
/// @endcode
inline void handle_exception(const std::exception& e) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
    throw;
}
#endif
