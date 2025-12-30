#define CR_HOST

#include <cr.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "Engine.hpp"
#include "Editor.hpp"
#include "DialogWindow.hpp"
#include "basicDialog.hpp"
#include "execute.hpp"

#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

std::string GetModuleHash(const std::string& path) {
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
        dlclose(lib);
    }
#endif
    return hash;
}

void RestartApplication(int argc, char* argv[]) {
    vex::log(">> Restarting Application...");
#ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        vex::log("Fatal: Could not get executable path.");
        exit(1);
    }

    std::string params = "";
    for (int i = 1; i < argc; i++) {
        params += "\"" + std::string(argv[i]) + "\" ";
    }

    ShellExecuteA(NULL, "open", exePath, params.c_str(), NULL, SW_SHOWDEFAULT);
    exit(0);

#else
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);

    if (len == -1) {
        perror("readlink failed");
        exit(1);
    }
    exePath[len] = '\0';

    execv(exePath, argv);
    perror("execv failed");
    exit(1);
#endif
}

void RebuildProject(const fs::path& projectPath, vex::GameInfo& info, bool clean) {
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
    if (argc < 2) {
        std::cerr << "Error: Missing argument.\n";
        std::cout << "Usage: " << argv[0] << " <path_to_project>\n";
        return 1;
    }

    vex::GameInfo dialogGameInfo("Dialog Window", 1, 0, 0);

    std::filesystem::path projectPath = argv[1];
    std::filesystem::path modulePath = projectPath / "Build" / "Debug";

    #ifdef __linux__
        std::string libPath = modulePath.string() + "/libGameModule.so";
    #else
        std::string libPath = modulePath.string() + "\\GameModule.dll";
    #endif

    if (!std::filesystem::exists(projectPath / "VexProject.json")) {
        createBasicDialog("Error", "No valid vex project file in path:\n" + projectPath.string(), dialogGameInfo);
        std::cerr << "Error: No valid vex project file in path: " << projectPath.string() << "\n";
        return 1;
    }

    if (!fs::exists(libPath)) {
        vex::log("Game Module missing. Building...");
        RebuildProject(projectPath, dialogGameInfo, false);
    }

    std::string engineHash = vex::Engine::GetBuildHash();
    std::string moduleHash = GetModuleHash(libPath);

    if (moduleHash.empty() || moduleHash != engineHash) {
        vex::log(">> ABI Mismatch Detected!");
        vex::log("   Engine Hash: %s", engineHash.c_str());
        vex::log("   Module Hash: %s", moduleHash.empty() ? "Unknown/Missing" : moduleHash.c_str());

        RebuildProject(projectPath, dialogGameInfo, true);
        RestartApplication(argc, argv);
        return 0;
    }

    cr_plugin ctx = {};
    std::string libName = "GameModule";

    vex::log("Attempting to load game module from: %s", libPath.c_str());

    if (!cr_plugin_open(ctx, libPath.c_str())) {
        createBasicDialog("Critical Error", "Failed to load Game Module! Check path and file permissions.\nAlternativly try clean building you project manually.", dialogGameInfo);
        vex::log("CRITICAL ERROR: Failed to load Game Module! Check path and file permissions.");
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
                vex::log(vex::LogLevel::ERROR, "CR Error: %d (Failure type: %d)", result, ctx.failure);
                vex::throw_error("Hot Reload Update Failed");
            }

            if (ctx.version != oldVersion) {
                engine.OnHotReload();
            }
        } catch (const std::exception& e) {
            vex::handle_critical_exception(e);
        }
    });

    //cr_plugin_close(ctx);
    std::quick_exit(0);
    return 0;
}
