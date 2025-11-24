#define CR_HOST
#include <cr.h>
#include <filesystem>
#include <iostream>
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

int main(int argc, char* argv[]) {
    vex::GameInfo gInfo{"VexGame", 0, 1, 0};
    vex::Engine engine("Vex Game", 1280, 720, gInfo);

    cr_plugin ctx = {};
    ctx.userdata = &engine;
    std::string buildDir = std::filesystem::current_path().string();
    std::string libName = "GameModule";

    #ifdef __linux__
         std::string libPath = buildDir + "/libGameModule.so";
    #else
        std::string libPath = buildDir + "/GameModule.dll";
    #endif

    vex::log("Attempting to load game module from: %s", libPath.c_str());

    if (!cr_plugin_open(ctx, libPath.c_str())) {
        vex::log("CRITICAL ERROR: Failed to load Game Module! Check path and file permissions.");
        return 1;
    }

    vex::log("Starting Engine Loop with Hot Reload...");

    engine.run([&]() {
            unsigned int oldVersion = ctx.version;

            int result = cr_plugin_update(ctx);

            if (result != 0) {
                vex::log("CR Error: %d (Failure type: %d)", result, ctx.failure);
                vex::throw_error("DAMN");
            }
    });

    cr_plugin_close(ctx);
    return 0;
}
