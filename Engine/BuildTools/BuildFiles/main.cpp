#define CR_HOST
#include <cr.h>
#include <filesystem>
#include <iostream>
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/GameInfo.hpp"

extern "C" void VexGame_Init(vex::Engine* engine);

int main(int argc, char* argv[]) {
    vex::GameInfo gInfo{VEX_PROJECT_TITLE, 0, 1, 0};
    vex::Engine engine(VEX_PROJECT_TITLE, 1280, 720, gInfo);

    #ifdef DIST_BUILD
        vex::InitCrashHandler();
        VexGame_Init(&engine);
        engine.run(nullptr);
    #else

    cr_plugin ctx = {};
    std::string buildDir = std::filesystem::current_path().string();
    std::string libName = "GameModule";

    #ifdef __linux__
         std::string libPath = buildDir + "/libGameModule.so";
    #else
        std::string libPath = buildDir + "\\GameModule.dll";
    #endif

    vex::log("Attempting to load game module from: %s", libPath.c_str());

    if (!cr_plugin_open(ctx, libPath.c_str())) {
        vex::log("CRITICAL ERROR: Failed to load Game Module! Check path and file permissions.");
        return 1;
    }

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
        } catch (const std::exception& e) {
            vex::handle_critical_exception(e);
        }
    });
#endif
    //cr_plugin_close(ctx);
    std::quick_exit(0);
    return 0;
}
