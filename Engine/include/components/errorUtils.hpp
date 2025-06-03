#pragma once
#include <stdexcept>
#include <iostream>
#include <SDL3/SDL_log.h>

#if ENABLE_CPPTRACE
#include <cpptrace/cpptrace.hpp>

inline void throw_error(const std::string& msg) {
    throw cpptrace::runtime_error(msg.c_str());
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
inline void throw_error(const std::string& msg) {
    throw std::runtime_error(msg.c_str());
}

inline void handle_exception(const std::exception& e) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
    throw;
}
#endif
