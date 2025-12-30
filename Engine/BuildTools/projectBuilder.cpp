#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h> // For MAXPATHLEN
#else // Linux
#include <unistd.h>
#endif

std::string GetCXXCompiler() {
    const char* env = std::getenv("VEX_CXX_COMPILER");
    return env ? std::string(env) : "clang++";
}

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

std::string GetEnginePath() {
    std::filesystem::path exeDir = GetExecutableDir();
    std::filesystem::path engineRoot = exeDir.parent_path().parent_path();

    return engineRoot.string();
}

std::string GetEngineCorePath() {
    std::filesystem::path exeDir = GetExecutableDir();
    std::filesystem::path engineRoot = exeDir.parent_path().parent_path();
    std::filesystem::path engineCorePath = engineRoot / "Core";

    return engineCorePath.string();
}

std::string ToCMakePath(const std::filesystem::path& path) {
    std::string p = path.string();
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

void BundleLinuxLibs(const std::filesystem::path& outputDir) {
    #ifdef __linux__
    std::cout << ">> Bundling Linux System Libraries for Portability...\n";

    std::vector<std::string> libs = {
        "libc++.so.1",
        "libc++abi.so.1",
        "libunwind.so.1"
    };

    for (const auto& libName : libs) {
        std::string cmd = "clang++ -print-file-name=" + libName + " > temp_lib_path.txt";
        if (std::system(cmd.c_str()) == 0) {
            std::ifstream file("temp_lib_path.txt");
            std::string pathStr;
            std::getline(file, pathStr);
            file.close();
            std::filesystem::remove("temp_lib_path.txt");

            pathStr.erase(pathStr.find_last_not_of(" \n\r\t") + 1);

            std::filesystem::path srcPath(pathStr);
            if (std::filesystem::exists(srcPath)) {
                std::filesystem::copy_file(srcPath, outputDir / libName, std::filesystem::copy_options::overwrite_existing);
                std::cout << "   [+] Bundled: " << libName << " (from " << srcPath.string() << ")\n";
            } else {
                std::cerr << "   [!] Warning: Could not locate " << libName << "\n";
            }
        }
    }
    #endif
}

void BundleWindowsDeps(const std::filesystem::path& outputDir) {
    std::cout << ">> Downloading Windows Dependencies...\n";


    std::filesystem::path depsDir = outputDir / "deps";

    if (!std::filesystem::exists(depsDir)) {
        std::filesystem::create_directories(depsDir);
    }

    std::string redistUrl = "https://aka.ms/vs/17/release/vc_redist.x64.exe";
    std::filesystem::path redistPath = depsDir / "vc_redist.x64.exe";

    std::string downloadCmd = "curl -L \"" + redistUrl + "\" -o \"" + redistPath.string() + "\"";

    std::cout << "   [+] Fetching Visual C++ Redistributable...\n";
    int result = std::system(downloadCmd.c_str());

    if (result == 0 && std::filesystem::exists(redistPath)) {
        std::cout << "   [+] Successfully downloaded to: " << redistPath.string() << "\n";
    } else {
        std::cerr << "   [!] Failed to download redistributable. Error code: " << result << "\n";
        std::string psCmd = "powershell -Command \"Invoke-WebRequest -Uri '" + redistUrl + "' -OutFile '" + redistPath.string() + "'\"";
        std::system(psCmd.c_str());
    }
}

void PerformSafeClean(const std::filesystem::path& projectDir) {
    std::cout << ">> [Clean] Cleaning Project Artifacts...\n";

    std::filesystem::path intermediate = projectDir / "Intermediate";
    if (std::filesystem::exists(intermediate)) {
        std::filesystem::remove_all(intermediate);
        std::cout << "   [+] Removed Intermediate\n";
    }

    std::filesystem::path buildDir = projectDir / "Build";
    if (std::filesystem::exists(buildDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(buildDir)) {
            if (entry.is_directory()) {
                std::string folderName = entry.path().filename().string();

                if (folderName != "Symbols") {
                    std::filesystem::remove_all(entry.path());
                    std::cout << "   [+] Removed Build/" << folderName << "\n";
                } else {
                    std::cout << "   [=] Preserved Build/Symbols\n";
                }
            }
        }
    }
}

bool EnsureSharedEngineBuilt(const std::filesystem::path& projectDir, const std::string& buildType, const std::string& parallel, bool is_dist = false) {


    std::string engineRelPath = GetEngineCorePath();
    if (engineRelPath.empty()) {
        std::cerr << "Error: Could not find engine path";
        return false;
    }

    std::filesystem::path enginePath = std::filesystem::weakly_canonical(projectDir / engineRelPath);

    std::string cmakeConfig = "Release";
    std::string outputDirName = "Release";

    if (buildType == "-d" || buildType == "-debug") {
        cmakeConfig = "Debug";
        outputDirName = "Debug";
    } else if (is_dist) {
        cmakeConfig = "Release";
        outputDirName = "Distribution";
    }

    std::filesystem::path engineBuildDir = enginePath / "bin" / outputDirName;
    std::filesystem::path targetFile = engineBuildDir / "VEXTargets.cmake";

    #ifdef __linux__
        std::filesystem::path libFile = engineBuildDir / "libVEX.so";
    #else
        std::filesystem::path libFile = engineBuildDir / "VEX.dll";
    #endif

    if (std::filesystem::exists(targetFile) && std::filesystem::exists(libFile)) {
        return true;
    }

    std::cout << ">> Shared VEX Engine (" << outputDirName << ") missing. Building it now...\n";
    std::cout << ">> Engine Path: " << enginePath.string() << "\n";

    std::filesystem::create_directories(engineBuildDir);

    std::filesystem::path cmakeSource = enginePath;

    std::string configCmd = "cmake -G Ninja -S \"" + cmakeSource.string() + "\" -B \"" + engineBuildDir.string() +
                            "\" -DCMAKE_BUILD_TYPE=" + cmakeConfig + " -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=" + GetCXXCompiler();

    if (is_dist) {
        configCmd += " -DVEX_DIST_BUILD=ON";
    }

    if (std::system(configCmd.c_str()) != 0) {
        std::cerr << "Engine Configuration Failed.\n";
        return false;
    }

    std::string buildCmd = "cmake --build \"" + engineBuildDir.string() + "\" --config " + cmakeConfig + " " + parallel;
    if (std::system(buildCmd.c_str()) != 0) {
        std::cerr << "Engine Build Failed.\n";
        return false;
    }

    std::cout << ">> Engine built successfully.\n";
    return true;
}

int main(int argc, char* argv[]) {


    // Set working directory to parent of executable
    try {
        std::filesystem::current_path(GetExecutableDir() / "..");
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to set working directory: " << e.what() << '\n';
        return 1;
    }

    unsigned int cores = std::thread::hardware_concurrency();
    std::string parallel = "--parallel " + std::to_string(cores > 0 ? cores : 1);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <project directory containing VexProject.json file> <build type -debug/-release (optional, defaults to debug)>\n";
        return 1;
    }

    std::filesystem::path project_dir = argv[1];
    std::string build_type = "-debug";
    bool is_dist = false;
    bool clean_build = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-debug" || arg == "-d") build_type = "-debug";
        else if (arg == "-release" || arg == "-r") build_type = "-release";
        else if (arg == "-dist" || arg == "-distribution") {
            build_type = "-release";
            is_dist = true;
        }
        else if (arg == "-clean" || arg == "--clean") clean_build = true;
    }

    std::filesystem::path vex_project_file = project_dir / "VexProject.json";
    if (!std::filesystem::exists(project_dir) || !std::filesystem::is_directory(project_dir) || !std::filesystem::exists(vex_project_file)) {
        std::cerr << "Project directory does not exist: \"" << project_dir.string() << "\" or does not contain VexProject.json\n";
        return 1;
    }

    if (!EnsureSharedEngineBuilt(project_dir, build_type, parallel, is_dist)) {
            return 1;
        }

    if (clean_build) {
        PerformSafeClean(project_dir);
    }

    std::filesystem::path intermediate_dir = project_dir / "Intermediate";
    if (!std::filesystem::exists(intermediate_dir) || !std::filesystem::is_directory(intermediate_dir)) {
        std::filesystem::create_directory(intermediate_dir);
    }

    try {
        std::filesystem::path build_path = project_dir / "build";
        std::filesystem::path Build_path = project_dir / "Build"; // Handle case sensitivity
        for (const auto& entry : std::filesystem::directory_iterator(project_dir)) {
            auto path = std::filesystem::weakly_canonical(entry.path());
            auto filename = path.filename().string();
            std::cout << intermediate_dir.string() << '\n';
            if (path != std::filesystem::weakly_canonical(build_path) &&
                path != std::filesystem::weakly_canonical(Build_path) &&
                path != std::filesystem::weakly_canonical(intermediate_dir)) {
                std::filesystem::path dest_path = intermediate_dir / filename;
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

    std::filesystem::path cmakelists_src = "BuildFiles/CMakeLists.txt";
    std::filesystem::path cmakelists_dest = intermediate_dir / "CMakeLists.txt";
    if (std::filesystem::exists(cmakelists_dest)) {
        std::filesystem::remove_all(cmakelists_dest);
    }
    std::filesystem::copy(cmakelists_src, cmakelists_dest);

    std::filesystem::path main_src = "BuildFiles/main.cpp";
    std::filesystem::path main_dest = intermediate_dir / "main.cpp";
    if (std::filesystem::exists(main_dest)) {
        std::filesystem::remove_all(main_dest);
    }
    std::filesystem::copy(main_src, main_dest);

    std::filesystem::path module_src = "BuildFiles/Source/GameEntry.cpp";
    std::filesystem::path module_dest = intermediate_dir / "Source" / "GameEntry.cpp";
    if (std::filesystem::exists(module_dest)) {
        std::filesystem::remove_all(module_dest);
    }
    std::filesystem::copy(module_src, module_dest);

    // Change working directory to Intermediate
    try {
        std::filesystem::current_path(intermediate_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to change directory: " << e.what() << '\n';
        return 1;
    }

    // Execute CMake configure
    std::filesystem::path build_dir = intermediate_dir / "build/debug";
    std::filesystem::path output_dir = project_dir / "Build";

    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directory(output_dir);
    }

    std::string engineArg = " -DVEX_ENGINE_PATH=\"" + ToCMakePath(GetEnginePath()) + "\"";

    if (is_dist) {
        engineArg += " -DVEX_DIST_BUILD=ON";
    }

    if (build_type == "-d" || build_type == "-debug") {
        output_dir = project_dir / "Build" / "Debug";
        std::string cmd = "cmake -G Ninja -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=" + GetCXXCompiler() + engineArg;
        std::cout << "CMD: " << cmd << std::endl;
        int result = std::system(cmd.c_str());
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

        std::string intermediate_build_dir = "build/release";

        if (is_dist) {
            output_dir = project_dir / "Build" / "Distribution";
            intermediate_build_dir = "build/dist";
            std::cout << ">> Building Distribution (Shipping).\n";
        } else {
            output_dir = project_dir / "Build" / "Release";
        }

        build_dir = intermediate_dir / intermediate_build_dir;
        std::string cmd = "cmake -G Ninja -S . -B " + intermediate_build_dir + " -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=" + GetCXXCompiler() + engineArg;

        std::cout << "CMD: " << cmd << std::endl;
        int result = std::system(cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake configure failed with exit code: " << result << '\n';
            return 1;
        }
        std::string build_cmd = "cmake --build " + intermediate_build_dir + " --config Release " + parallel;
        result = std::system(build_cmd.c_str());
        if (result != 0) {
            std::cerr << "CMake build failed with exit code: " << result << '\n';
            return 1;
        }
    }

    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directory(output_dir);
    } else {
        std::filesystem::remove_all(output_dir);
        std::filesystem::create_directory(output_dir);
    }

    try {
        std::filesystem::path cpack_source = build_dir / "CPackSourceConfig.cmake";
        std::filesystem::path cpack_config = build_dir / "CPackConfig.cmake";
        std::filesystem::path deps = build_dir / "_deps";
        std::filesystem::path cmake_files = build_dir / "CMakeFiles";
        std::filesystem::path cmake_cache = build_dir / "CMakeCache.txt";
        std::filesystem::path cmake_cache_dir = build_dir / ".cache";
        std::filesystem::path cmake_install = build_dir / "cmake_install.cmake";
        std::filesystem::path compile_commands = build_dir / "compile_commands.json";
        std::filesystem::path ninja_deps = build_dir / ".ninja_deps";
        std::filesystem::path ninja_log = build_dir / ".ninja_log";
        std::filesystem::path build_ninja = build_dir / "build.ninja";
        std::filesystem::path yoga_bin = build_dir / "bin";
        std::filesystem::path yoga_lib = build_dir / "lib";
        std::filesystem::path yoga_tests = build_dir / "yogatests";

        for (const auto& entry : std::filesystem::directory_iterator(build_dir)) {
            auto path = std::filesystem::weakly_canonical(entry.path());
            auto filename = path.filename().string();
            if (path != std::filesystem::weakly_canonical(cpack_source) &&
                path != std::filesystem::weakly_canonical(cpack_config) &&
                path != std::filesystem::weakly_canonical(deps) &&
                path != std::filesystem::weakly_canonical(cmake_files) &&
                path != std::filesystem::weakly_canonical(cmake_cache) &&
                path != std::filesystem::weakly_canonical(cmake_cache_dir) &&
                path != std::filesystem::weakly_canonical(cmake_install) &&
                path != std::filesystem::weakly_canonical(compile_commands) &&
                path != std::filesystem::weakly_canonical(ninja_deps) &&
                path != std::filesystem::weakly_canonical(ninja_log) &&
                path != std::filesystem::weakly_canonical(build_ninja) &&
                path != std::filesystem::weakly_canonical(yoga_bin) &&
                path != std::filesystem::weakly_canonical(yoga_lib) &&
                path != std::filesystem::weakly_canonical(yoga_tests) &&
                filename.find("cmake") == std::string::npos) {
                std::filesystem::path dest_path = output_dir / filename;
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

    std::string engine_rel_path = GetEngineCorePath();
    std::filesystem::path engine_root = std::filesystem::weakly_canonical(project_dir / engine_rel_path);

    std::string config_name = (build_type == "-d" || build_type == "-debug") ? "Debug" : "Release";

    std::string engine_bin_config = config_name;
    if (is_dist) {
        engine_bin_config = "Distribution";
    }
    std::filesystem::path engine_bin_dir = engine_root / "bin" / engine_bin_config;

    std::cout << ">> Linking Shared Engine artifacts from: " << engine_bin_dir.string() << "\n";

    if (std::filesystem::exists(engine_bin_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(engine_bin_dir)) {
            const auto& src_path = entry.path();

                if (std::filesystem::is_directory(src_path) && src_path.filename() == "shaders") {
                    std::filesystem::path dest_shaders = output_dir / "Engine/shaders";

                    if (std::filesystem::exists(dest_shaders)) {
                        std::filesystem::remove_all(dest_shaders);
                    }
                    std::filesystem::create_directories(dest_shaders);

                    std::filesystem::copy(src_path, dest_shaders, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                    std::cout << "   [+] Copied Shaders\n";
                }
                else if (std::filesystem::is_regular_file(src_path)) {
                    std::string ext = src_path.extension().string();
                    if (src_path.string().find(".so") != std::string::npos ||
                        ext == ".dll" || ext == ".lib" || ext == ".pdb" || ext == ".dylib") {
                        std::filesystem::path dest_file = output_dir / src_path.filename();
                        if (is_dist && (ext == ".pdb" || ext == ".debug" || ext == ".ilk" || ext == ".exp")) {
                            continue;
                        }
                        std::filesystem::copy_file(src_path, dest_file, std::filesystem::copy_options::overwrite_existing);
                        std::cout << "   [+] Copied " << src_path.filename().string() << "\n";
                    }
                }
            }
        } else {
            std::cerr << ">> CRITICAL WARNING: Engine binary directory not found at " << engine_bin_dir.string() << "\n";
            std::cerr << ">> The game will likely fail to launch.\n";
            return 1;
        }

    try {
        std::filesystem::path build_path = intermediate_dir / "build";
        for (const auto& entry : std::filesystem::directory_iterator(intermediate_dir)) {
            auto path = std::filesystem::weakly_canonical(entry.path());
            if (path != std::filesystem::weakly_canonical(build_path)) {
                std::filesystem::remove_all(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    if (is_dist) {
        #if defined(__linux__)
        BundleLinuxLibs(output_dir);
        #elif defined(_WIN32)
        BundleWindowsDeps(output_dir);
        #endif
    }

    return 0;
}
