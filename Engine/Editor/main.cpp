#include <X11/X.h>
#define CR_HOST

#include <cr.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>

#include "Engine.hpp"
#include "Editor.hpp"
#include "DialogWindow.hpp"
#include "basicDialog.hpp"
#include "execute.hpp"

#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Missing argument.\n";
        std::cout << "Usage: " << argv[0] << " <path_to_project>\n";
        return 1;
    }

    vex::GameInfo dialogGameInfo("Dialog Window", 1, 0, 0);

    fs::path projectPath = argv[1];
    fs::path modulePath = projectPath / "Build" / "Debug";

    #ifdef __linux__
        std::string libPath = modulePath.string() + "/libGameModule.so";
    #else
        std::string libPath = modulePath.string() + "\\GameModule.dll";
    #endif

    if (!fs::exists(projectPath / "VexProject.json")) {
        createBasicDialog("Error", "No valid vex project file in path:\n" + projectPath.string(), dialogGameInfo);
        std::cerr << "Error: No valid vex project file in path: " << projectPath.string() << "\n";
        return 1;
    }

    if (!fs::exists(fs::path(libPath))) {
        vex::log("Info: Could not find game module: %s\nRebuilding...", libPath.c_str());

        vex::DialogWindow dialogWindow("Building Project...", dialogGameInfo);

        std::atomic<bool> ui_ready{false};
        std::thread ui_thread([&]() {
                ui_ready.store(true);
                dialogWindow.run();
                vex::log("Info: Dialog window UI thread finished.");
            });

            while (!ui_ready.load()) {
                std::this_thread::yield();
            }

        std::string command;
        std::string outputLog;

        #ifdef _WIN32
            command = "..\\..\\BuildTools\\build\\ProjectBuilder.exe \"" + projectPath.string() + "\" -d";
        #else
            command = "../../BuildTools/build/ProjectBuilder \"" + projectPath.string() + "\" -d";
        #endif

        vex::log("Executing command: %s", command.c_str());

        executeCommandRealTime(command,
            [&](const std::string& line) {
                outputLog += line;
                std::cout << line;
                dialogWindow.setDialogContent(outputLog);
            }
        );
        dialogWindow.stop();
        ui_thread.join();
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
            vex::handle_exception(e);
        }
    });

    //cr_plugin_close(ctx);
    std::quick_exit(0);
    return 0;
}
