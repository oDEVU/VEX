#define CR_HOST
#include <cr.h>
#include <filesystem>
#include <iostream>
#include "Engine.hpp"
#include "Editor.hpp"
#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Missing argument.\n";
        std::cout << "Usage: " << argv[0] << " <path_to_project>\n";
        return 1;
    }

    fs::path projectPath = argv[1];

    fs::path modulePath = projectPath / "Build" / "Debug";

    #ifdef __linux__
        std::string libPath = modulePath.string() + "/libGameModule.so";
    #else
        std::string libPath = modulePath.string() + "\\GameModule.dll";
    #endif

    if (!fs::exists(projectPath / "VexProject.json")) {
        std::cerr << "Error: No valid vex project file in path: " << projectPath.string() << "\n";
        return 1;
    }

    cr_plugin ctx = {};
    std::string libName = "GameModule";

    vex::log("Attempting to load game module from: %s", libPath.c_str());

    if (!cr_plugin_open(ctx, libPath.c_str())) {
        vex::log("CRITICAL ERROR: Failed to load Game Module! Check path and file permissions.");
        return 1;
    }

    vex::GameInfo gInfo{"VEX Editor", 0, 1, 0};
    vex::Editor engine("VEX Editor", 1280, 720, gInfo, modulePath.string());
    ctx.userdata = &engine;

    vex::log("Starting Engine Loop with Hot Reload...");

    engine.run([&]() {
            unsigned int oldVersion = ctx.version;

            int result = cr_plugin_update(ctx);

            if (result != 0) {
                vex::log("CR Error: %d (Failure type: %d)", result, ctx.failure);
                vex::throw_error("DAMN");
            }
    });

    //cr_plugin_close(ctx);
    std::quick_exit(0);
    return 0;
}
