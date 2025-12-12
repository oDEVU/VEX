#include "ProjectSelector/ProjectSelector.hpp"
#include <filesystem>
#include <iostream>
#include <vector>

#include "components/errorUtils.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #include <direct.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

std::filesystem::path GetSelfDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        return std::filesystem::path(std::string(result, count)).parent_path();
    }
    return std::filesystem::current_path();
#endif
}

int main(int argc, char* argv[]) {
    std::string selectedPath;

    {
        try {
        vex::ProjectSelector selector("Vex Project Selector");
        selector.run([&]() {
            selectedPath = selector.getSelectedProject();
        });
        } catch (const std::exception& e) {
            vex::handle_exception(e);
        }
    }

    if (selectedPath.empty()) {
        vex::log(vex::LogLevel::ERROR, "Project path was not passed!");
        return 0;
    }

    std::filesystem::path binDir = GetSelfDir();

#ifdef _WIN32
    std::string exeName = "VexEditor.exe";
    std::filesystem::path fullPath = binDir / exeName;

    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "[Error] Could not find editor at: " << fullPath << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    const char* args[] = { fullPath.string().c_str(), selectedPath.c_str(), NULL };
    _spawnv(_P_OVERLAY, fullPath.string().c_str(), args);
#else
    std::string exeName = "VexEditor";
    std::filesystem::path fullPath = binDir / exeName;

    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "[Error] Could not find editor at: " << fullPath << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    char* args[] = {
        (char*)exeName.c_str(),
        (char*)selectedPath.c_str(),
        NULL
    };

    execv(fullPath.c_str(), args);

    perror("execv failed");
    std::cerr << "[Fatal] Failed to launch: " << fullPath << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    return 1;
#endif

    return 0;
}
