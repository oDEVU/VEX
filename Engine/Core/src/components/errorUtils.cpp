#include "components/errorUtils.hpp"

#include <iostream>
#include <SDL3/SDL_log.h>
#include <stdio.h>
#include <stdarg.h>

// Includujemy cpptrace tylko w implementacji
#if DEBUG
#include <cpptrace/cpptrace.hpp>
#endif

namespace vex {

#if DEBUG

    [[noreturn]] void throw_error(const std::string& msg) {
        throw cpptrace::runtime_error(msg.c_str());
    }

    void log(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
        va_end(args);
    }

    void handle_exception(const std::exception& e) {
        if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
            cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
        } else {
            SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        }
        throw;
    }

#else
    // WERSJA RELEASE

    [[noreturn]] void throw_error(const std::string& msg) {
        throw std::runtime_error(msg.c_str());
    }

    void log(const char* fmt, ...) {
        // W release logi mogą być wyłączone lub uproszczone
        return;
    }

    void handle_exception(const std::exception& e) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        throw;
    }

#endif

} // namespace vex
