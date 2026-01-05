#pragma once
#include <iostream>
#include <string>
#include <cstdio>
#include <functional>

#ifdef _WIN32
    #define POPEN_FUNC _popen
    #define PCLOSE_FUNC _pclose
#else
    #define POPEN_FUNC popen
    #define PCLOSE_FUNC pclose
    #include <unistd.h>
#endif

/// @brief Executes a command in real-time and calls a callback function for each line of output.
/// @param const std::string& cmd - Command to execute.
/// @param std::function<void(const std::string&)> lineCallback - Callback function to call for each line of output.
inline void executeCommandRealTime(const std::string& cmd, std::function<void(const std::string&)> lineCallback) {
    FILE* pipe = POPEN_FUNC(cmd.c_str(), "r");

    if (!pipe) {
        lineCallback("ERROR: Could not run command: " + cmd);
        return;
    }

    setbuf(pipe, NULL);
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line = buffer;
        lineCallback(line);
        std::fflush(stdout);
    }

    PCLOSE_FUNC(pipe);
}
