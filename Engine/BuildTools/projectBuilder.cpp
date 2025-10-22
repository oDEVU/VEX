#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstring>
#include <filesystem>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h> // For MAXPATHLEN
#else // Linux
#include <unistd.h>
#endif

// Copied from pathUtils
inline std::filesystem::path GetExecutableDir() {
    std::filesystem::path exePath;

#ifdef _WIN32
    // Windows: Use GetModuleFileNameW
    wchar_t wPath[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, wPath, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        std::cerr << "Failed to get executable path on Windows";
    }
    // Convert UTF-16 to UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wPath, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0) {
        std::cerr << "Failed to convert executable path to UTF-8";
    }
    std::string utf8Path(utf8Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wPath, -1, utf8Path.data(), utf8Size, nullptr, nullptr);
    exePath = std::filesystem::path(utf8Path);
#elif defined(__APPLE__)
    // macOS: Use _NSGetExecutablePath
    char pathBuf[MAXPATHLEN];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) != 0) {
        std::cerr << "Failed to get executable path on macOS";
    }
    exePath = std::filesystem::canonical(pathBuf); // Resolve symlinks/aliases
#else
    // Linux: Use readlink
    char pathBuf[1024];
    ssize_t len = ::readlink("/proc/self/exe", pathBuf, sizeof(pathBuf) - 1);
    if (len == -1) {
        std::cerr << "Failed to read /proc/self/exe on Linux";
    }
    pathBuf[len] = '\0';
    exePath = std::filesystem::path(pathBuf);
#endif

    return exePath.parent_path();
}

int main(int argc, char* argv[]) {

    std::filesystem::current_path(GetExecutableDir().string() + "/../");

    unsigned int cores = std::thread::hardware_concurrency();
    std::string parallel = "--parallel " + std::to_string(cores > 0 ? cores : 1);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <project directory containing VexProject.json file> <build type -debug/-release (optional, defaults to debug)>" << std::endl;
        return 1;
    }

    std::string project_dir = argv[1];
    std::string build_type = "-debug";

    if (argc >= 3) {
            std::string arg2 = argv[2];
            if (arg2 == "-debug" || arg2 == "-release" || arg2 == "-d" || arg2 == "-r") {
                build_type = arg2;
            } else {
                std::cout << "Invalid build type '" << arg2 << "'. Defaulting to debug." << std::endl;
            }
        }

    if (!std::filesystem::exists(project_dir) || !std::filesystem::is_directory(project_dir) || !std::filesystem::exists(project_dir + "/VexProject.json")) {
        std::cerr << "Project directory does not exist: \"" << project_dir << "\" or does not contain VexProject.json file" << std::endl;
        return 1;
    }

    std::string intermediate_dir = project_dir + "/Intermediate";
    if (!std::filesystem::exists(intermediate_dir) || !std::filesystem::is_directory(intermediate_dir)) {
        std::filesystem::create_directory(intermediate_dir);
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(project_dir)) {
            std::string path_str = entry.path().string();
            std::string filename = entry.path().filename().string();
            if (path_str != project_dir + "/build" && path_str != project_dir + "/Build" && path_str != intermediate_dir) {
                std::string dest_path = intermediate_dir + "/" + filename;
                if (std::filesystem::exists(dest_path)) {
                    std::filesystem::remove_all(dest_path);
                }
                std::filesystem::copy(entry.path(), dest_path, std::filesystem::copy_options::recursive);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    if (std::filesystem::exists(intermediate_dir + "/CMakeLists.txt")) {
        std::filesystem::remove_all(intermediate_dir + "/CMakeLists.txt");
    }
    std::filesystem::copy("BuildFiles/CMakeLists.txt", intermediate_dir + "/CMakeLists.txt");

    // Change current working directory to Intermediate
    try {
        std::filesystem::current_path(intermediate_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to change directory: " << e.what() << '\n';
        return 1;
    }

    // Execute cmake configure
    std::string build_dir = intermediate_dir + "/build/debug";
    std::string output_dir = project_dir + "/Build";

    if (!std::filesystem::exists(output_dir)) {
         std::filesystem::create_directory(output_dir);
    }

    if(build_type == "-d" || build_type == "-debug"){
        output_dir = project_dir + "/Build/Debug";
        int result = std::system("cmake -G Ninja -B build/debug -DCMAKE_BUILD_TYPE=Debug");
        if (result != 0) {
            std::cerr << "CMake configure failed with exit code: " << result << std::endl;
            return 1;
        }
        std::string build_cmd = "cmake --build build/debug --config Debug " + parallel;
        result = std::system(build_cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake build failed with exit code: " << result << std::endl;
            return 1;
        }
    }else if(build_type == "-r" || build_type == "-release"){
        output_dir = project_dir + "/Build/Release";
        build_dir = intermediate_dir + "/build/release";
        int result = std::system("cmake -G Ninja -B build/release -DCMAKE_BUILD_TYPE=Release");
        if (result != 0) {
            std::cerr << "CMake configure failed with exit code: " << result << std::endl;
            return 1;
        }
        std::string build_cmd = "cmake --build build/release --config Release " + parallel;
        result = std::system(build_cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake build failed with exit code: " << result << std::endl;
            return 1;
        }
    }

    if (!std::filesystem::exists(output_dir)) {
         std::filesystem::create_directory(output_dir);
    }else{
        std::filesystem::remove_all(output_dir);
        std::filesystem::create_directory(output_dir);
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(build_dir)) {
            std::string path_str = entry.path().string();
            std::string filename = entry.path().filename().string();

            // Build files to ignore while comping
            if (path_str != build_dir + "/CPackSourceConfig.cmake"
                && path_str != build_dir + "/CPackConfig.cmake"
                && path_str != build_dir + "/_deps"
                && path_str != build_dir + "/CMakeFiles"
                && path_str != build_dir + "/CMakeCache.txt"
                && path_str != build_dir + "/cmake_install.cmake"
                && path_str != build_dir + "/compile_commands.json"
                && path_str != build_dir + "/.ninja_deps"
                && path_str != build_dir + "/.ninja_log"
                && path_str != build_dir + "/build.ninja"
                && path_str.find("cmake")) {

                std::string dest_path = output_dir + "/" + filename;
                if (std::filesystem::exists(dest_path)) {
                    std::filesystem::remove_all(dest_path);
                }
                std::filesystem::copy(entry.path(), dest_path, std::filesystem::copy_options::recursive);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    output_dir = output_dir + "/Engine";

        try {
            for (const auto& entry : std::filesystem::directory_iterator(output_dir)) {
                std::string path_str = entry.path().string();
                std::string filename = entry.path().filename().string();
                if (path_str != output_dir + "/libVEX.a" && path_str != output_dir + "/shaders") {
                    std::filesystem::remove_all(entry.path());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }

                try {
                    for (const auto& entry : std::filesystem::directory_iterator(intermediate_dir)) {
                        std::string path_str = entry.path().string();
                        std::string filename = entry.path().filename().string();
                        if (path_str != intermediate_dir + "/build") {
                            std::filesystem::remove_all(entry.path());
                        }
                    }
                } catch (const std::filesystem::filesystem_error& e) {
                    std::cerr << e.what() << '\n';
                    return 1;
                }
    return 0;
}
