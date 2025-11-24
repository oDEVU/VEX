/**
 *  @file   errorUtils.hpp
 *  @brief  This file defines functions for logging and throwing errors. It provides error tracing for debugging purposes that are not present for release builds.
 *  @author Eryk Roszkowski
 ***********************************************/

 #pragma once
 #include <stdexcept>
 #include <string>
 #include "VEX/VEX_export.h"

 namespace vex {

/// @brief Throws an error with the given message and triggers cpptrace for debug builds.
/// @details Example usage:
/// @code
///     throw_error("An error occurred");
/// @endcode
[[noreturn]] void VEX_EXPORT throw_error(const std::string& msg);

/// @brief Logs a message with the given format and arguments to the console, or does absolutely nothing for release builds.
/// @details Example usage:
/// @code
///     log("value a is %d", a);
/// @endcode
void VEX_EXPORT log(const char* fmt, ...);

/// @brief Handles an exception by logging it and rethrowing it.
/// @details Example usage:
/// @code
/// try {
///     something();
/// } catch (const std::exception& e) {
///     handle_exception(e);
/// }
/// @endcode
void VEX_EXPORT handle_exception(const std::exception& e);

 }
