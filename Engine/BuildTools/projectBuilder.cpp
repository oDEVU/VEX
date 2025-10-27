#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h> // For MAXPATHLEN
#else // Linux
#include <unistd.h>
#endif

// Get directory containing the executable
inline std::filesystem::path GetExecutableDir() {
    std::filesystem::path exePath;

#ifdef _WIN32
    // Windows: Use GetModuleFileNameW
    wchar_t wPath[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, wPath, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        std::cerr << "Failed to get executable path on Windows\n";
        return {};
    }
    // Convert UTF-16 to UTF-8
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wPath, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0) {
        std::cerr << "Failed to convert executable path to UTF-8\n";
        return {};
    }
    std::string utf8Path(utf8Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wPath, -1, utf8Path.data(), utf8Size, nullptr, nullptr);
    exePath = std::filesystem::path(utf8Path);
#elif defined(__APPLE__)
    // macOS: Use _NSGetExecutablePath
    char pathBuf[MAXPATHLEN];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) != 0) {
        std::cerr << "Failed to get executable path on macOS\n";
        return {};
    }
    exePath = std::filesystem::canonical(pathBuf);
#else
    // Linux: Use readlink
    char pathBuf[1024];
    ssize_t len = ::readlink("/proc/self/exe", pathBuf, sizeof(pathBuf) - 1);
    if (len == -1) {
        std::cerr << "Failed to read /proc/self/exe on Linux\n";
        return {};
    }
    pathBuf[len] = '\0';
    exePath = std::filesystem::path(pathBuf);
#endif

    return exePath.parent_path();
}

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    // Set working directory to parent of executable
    try {
        fs::current_path(GetExecutableDir() / "..");
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to set working directory: " << e.what() << '\n';
        return 1;
    }

    unsigned int cores = std::thread::hardware_concurrency();
    std::string parallel = "--parallel " + std::to_string(cores > 0 ? cores : 1);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <project directory containing VexProject.json file> <build type -debug/-release (optional, defaults to debug)>\n";
        return 1;
    }

    fs::path project_dir = argv[1];
    std::string build_type = "-debug";

    if (argc >= 3) {
        std::string arg2 = argv[2];
        if (arg2 == "-debug" || arg2 == "-release" || arg2 == "-d" || arg2 == "-r") {
            build_type = arg2;
        } else {
            std::cout << "Invalid build type '" << arg2 << "'. Defaulting to debug.\n";
        }
    }

    fs::path vex_project_file = project_dir / "VexProject.json";
    if (!fs::exists(project_dir) || !fs::is_directory(project_dir) || !fs::exists(vex_project_file)) {
        std::cerr << "Project directory does not exist: \"" << project_dir.string() << "\" or does not contain VexProject.json\n";
        return 1;
    }

    fs::path intermediate_dir = project_dir / "Intermediate";
    if (!fs::exists(intermediate_dir) || !fs::is_directory(intermediate_dir)) {
        fs::create_directory(intermediate_dir);
    }

    try {
        fs::path build_path = project_dir / "build";
        fs::path Build_path = project_dir / "Build"; // Handle case sensitivity
        for (const auto& entry : fs::directory_iterator(project_dir)) {
            auto path = fs::weakly_canonical(entry.path());
            auto filename = path.filename().string();
            std::cout << intermediate_dir.string() << '\n';
            if (path != fs::weakly_canonical(build_path) &&
                path != fs::weakly_canonical(Build_path) &&
                path != fs::weakly_canonical(intermediate_dir)) {
                fs::path dest_path = intermediate_dir / filename;
                if (fs::exists(dest_path)) {
                    fs::remove_all(dest_path);
                }
                fs::copy(entry.path(), dest_path, fs::copy_options::recursive);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    fs::path cmakelists_src = "BuildFiles/CMakeLists.txt";
    fs::path cmakelists_dest = intermediate_dir / "CMakeLists.txt";
    if (fs::exists(cmakelists_dest)) {
        fs::remove_all(cmakelists_dest);
    }
    fs::copy(cmakelists_src, cmakelists_dest);

    // Change working directory to Intermediate
    try {
        fs::current_path(intermediate_dir);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to change directory: " << e.what() << '\n';
        return 1;
    }

    // Execute CMake configure
    fs::path build_dir = intermediate_dir / "build/debug";
    fs::path output_dir = project_dir / "Build";

    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    }

    if (build_type == "-d" || build_type == "-debug") {
        output_dir = project_dir / "Build" / "Debug";
        int result = std::system("cmake -G Ninja -B build/debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++");
        if (result != 0) {
            std::cerr << "CMake configure failed with exit code: " << result << '\n';
            return 1;
        }
        std::string build_cmd = "cmake --build build/debug --config Debug " + parallel;
        result = std::system(build_cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake build failed with exit code: " << result << '\n';
            return 1;
        }
    } else if (build_type == "-r" || build_type == "-release") {
        output_dir = project_dir / "Build" / "Release";
        build_dir = intermediate_dir / "build/release";
        int result = std::system("cmake -G Ninja -B build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++");
        if (result != 0) {
            std::cerr << "CMake configure failed with exit code: " << result << '\n';
            return 1;
        }
        std::string build_cmd = "cmake --build build/release --config Release " + parallel;
        result = std::system(build_cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake build failed with exit code: " << result << '\n';
            return 1;
        }
    }

    if (!fs::exists(output_dir)) {
        fs::create_directory(output_dir);
    } else {
        fs::remove_all(output_dir);
        fs::create_directory(output_dir);
    }

    try {
        fs::path cpack_source = build_dir / "CPackSourceConfig.cmake";
        fs::path cpack_config = build_dir / "CPackConfig.cmake";
        fs::path deps = build_dir / "_deps";
        fs::path cmake_files = build_dir / "CMakeFiles";
        fs::path cmake_cache = build_dir / "CMakeCache.txt";
        fs::path cmake_install = build_dir / "cmake_install.cmake";
        fs::path compile_commands = build_dir / "compile_commands.json";
        fs::path ninja_deps = build_dir / ".ninja_deps";
        fs::path ninja_log = build_dir / ".ninja_log";
        fs::path build_ninja = build_dir / "build.ninja";

        for (const auto& entry : fs::directory_iterator(build_dir)) {
            auto path = fs::weakly_canonical(entry.path());
            auto filename = path.filename().string();
            if (path != fs::weakly_canonical(cpack_source) &&
                path != fs::weakly_canonical(cpack_config) &&
                path != fs::weakly_canonical(deps) &&
                path != fs::weakly_canonical(cmake_files) &&
                path != fs::weakly_canonical(cmake_cache) &&
                path != fs::weakly_canonical(cmake_install) &&
                path != fs::weakly_canonical(compile_commands) &&
                path != fs::weakly_canonical(ninja_deps) &&
                path != fs::weakly_canonical(ninja_log) &&
                path != fs::weakly_canonical(build_ninja) &&
                filename.find("cmake") == std::string::npos) {
                fs::path dest_path = output_dir / filename;
                if (fs::exists(dest_path)) {
                    fs::remove_all(dest_path);
                }
                fs::copy(entry.path(), dest_path, fs::copy_options::recursive);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    output_dir = output_dir / "Engine";

    try {
        fs::path libvex_path = output_dir / "libVEX.a";
        fs::path shaders_path = output_dir / "shaders";
        for (const auto& entry : fs::directory_iterator(output_dir)) {
            auto path = fs::weakly_canonical(entry.path());
            if (path != fs::weakly_canonical(libvex_path) &&
                path != fs::weakly_canonical(shaders_path)) {
                fs::remove_all(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    try {
        fs::path build_path = intermediate_dir / "build";
        for (const auto& entry : fs::directory_iterator(intermediate_dir)) {
            auto path = fs::weakly_canonical(entry.path());
            if (path != fs::weakly_canonical(build_path)) {
                fs::remove_all(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
