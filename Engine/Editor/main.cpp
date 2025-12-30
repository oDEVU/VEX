#define CR_HOST

#include <cr.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <cstdio>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#endif

#include "Engine.hpp"
#include "Editor.hpp"
#include "DialogWindow.hpp"
#include "basicDialog.hpp"
#include "execute.hpp"

#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

std::filesystem::path GetSelfPath() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) return "";
    return std::filesystem::path(path);
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count == -1) return "";
    return std::filesystem::path(std::string(path, count));
#endif
}

void RestartApplication(int argc, char* argv[]) {
    vex::log(">> Restarting Application...");
    std::filesystem::path exePath = GetSelfPath();
    if (exePath.empty()) exit(1);

#ifdef _WIN32
    std::string params = "";
    for (int i = 1; i < argc; i++) params += "\"" + std::string(argv[i]) + "\" ";
    ShellExecuteA(NULL, "open", exePath.string().c_str(), params.c_str(), NULL, SW_SHOWDEFAULT);
    exit(0);
#else
    execv(exePath.c_str(), argv);
    exit(1);
#endif
}

void CheckHashAndExit(const std::string& path) {
    std::string hash = "0";
#ifdef _WIN32
    HMODULE lib = LoadLibraryExA(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (lib) {
        typedef const char* (*FuncType)();
        FuncType func = (FuncType)GetProcAddress(lib, "VexModule_GetExpectedHash");
        if (func) {
            hash = func();
        } else {
            std::cerr << ">> [Warning] GameModule is missing hash export (VexModule_GetExpectedHash).\n"
                      << "             This usually means it was built with an older Engine version.\n";
        }
        FreeLibrary(lib);
    }
#else
    void* lib = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (lib) {
        typedef const char* (*FuncType)();
        FuncType func = (FuncType)dlsym(lib, "VexModule_GetExpectedHash");
        if (func) {
            hash = func();
        } else {
            std::cerr << ">> [Warning] GameModule is missing hash export (VexModule_GetExpectedHash).\n"
                      << "             This usually means it was built with an older Engine version.\n";
        }
    }
#endif
    std::cout << hash << std::flush;
    std::quick_exit(0);
}

std::string GetModuleHash(const std::string& libPath) {
    std::filesystem::path exe = GetSelfPath();
    if (exe.empty()) return "0";

    #ifdef _WIN32
        std::string cmd = "\"\"" + exe.string() + "\" --check-hash \"" + libPath + "\"\"";
    #else
        std::string cmd = "\"" + exe.string() + "\" --check-hash \"" + libPath + "\"";
    #endif
    std::string result = "0";

#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe) {
        vex::log("Failed to spawn hash checker process.");
        return "0";
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}

void RebuildProject(const std::filesystem::path& projectPath, vex::GameInfo& info, bool clean) {
    vex::DialogWindow dialogWindow(clean ? "Engine Updated - Clean Rebuilding..." : "Building Project...", info);

    std::atomic<bool> ui_ready{false};
    std::thread ui_thread([&]() {
        ui_ready.store(true);
        dialogWindow.run();
    });

    while (!ui_ready.load()) std::this_thread::yield();

    std::string command;
    std::string cleanFlag = clean ? " -clean" : "";

    #ifdef _WIN32
        command = "..\\..\\BuildTools\\build\\ProjectBuilder.exe \"" + projectPath.string() + "\" -d" + cleanFlag;
    #else
        command = "../../BuildTools/build/ProjectBuilder \"" + projectPath.string() + "\" -d" + cleanFlag;
    #endif

    std::string outputLog;
    executeCommandRealTime(command, [&](const std::string& line) {
        outputLog += line;
        std::cout << line;
        dialogWindow.setDialogContent(outputLog);
    });

    dialogWindow.stop();
    ui_thread.join();
}

int main(int argc, char* argv[]) {
    if (argc == 3 && std::string(argv[1]) == "--check-hash") {
        CheckHashAndExit(argv[2]);
        return 0;
    }

    if (argc < 2) {
        std::cerr << "Error: Missing argument.\n";
        std::cout << "Usage: " << argv[0] << " <path_to_project>\n";
        return 1;
    }

    vex::GameInfo dialogGameInfo("Dialog Window", 1, 0, 0);

    std::filesystem::path projectPath = argv[1];
    std::filesystem::path modulePath = projectPath / "Build" / "Debug";

    #ifdef __linux__
        std::string libPath = (modulePath / "libGameModule.so").string();
    #else
        std::string libPath = (modulePath / "GameModule.dll").string();
    #endif

    if (!std::filesystem::exists(projectPath / "VexProject.json")) {
        createBasicDialog("Error", "No valid vex project file in path:\n" + projectPath.string(), dialogGameInfo);
        vex::log(vex::LogLevel::ERROR, "No valid vex project file in path: %s", projectPath.string().c_str());
        return 1;
    }

    if (!std::filesystem::exists(libPath)) {
        vex::log("Game Module missing. Building...");
        RebuildProject(projectPath, dialogGameInfo, false);
    }

    std::string engineHash = vex::Engine::GetBuildHash();
    std::string moduleHash = GetModuleHash(libPath);

    if (moduleHash.empty() || moduleHash == "0" || moduleHash != engineHash) {
        vex::log(">> ABI Mismatch Detected!");
        vex::log("   Engine Hash: %s", engineHash.c_str());
        vex::log("   Module Hash: %s", moduleHash.c_str());

        RebuildProject(projectPath, dialogGameInfo, true);
        RestartApplication(argc, argv);
        return 0;
    }

    cr_plugin ctx = {};

    vex::log("Attempting to load game module from: %s", libPath.c_str());

    if (!cr_plugin_open(ctx, libPath.c_str())) {
        createBasicDialog("Critical Error", "Failed to load Game Module! Check path and file permissions.\nAlternativly try clean building you project manually.", dialogGameInfo);
        vex::log(vex::LogLevel::CRITICAL, "Failed to load Game Module! Check path and file permissions.");
        return 1;
    }

    vex::GameInfo gInfo{"VEX Editor", 0, 1, 0};
    vex::Editor engine("VEX Editor", 1280, 720, gInfo, projectPath.string());
    ctx.userdata = &engine;

    vex::InitCrashHandler();

    vex::log("Starting Engine Loop with Hot Reload...");

    engine.run([&]() {
        try {
            unsigned int oldVersion = ctx.version;
            int result = cr_plugin_update(ctx);

            if (result != 0) {
                vex::throw_error("Hot Reload Update Failed");
            }

            if (ctx.version != oldVersion) {
                engine.OnHotReload();
            }
        } catch (const std::exception& e) {
            vex::handle_critical_exception(e);
        }
    });

    std::quick_exit(0);
    return 0;
}
