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
