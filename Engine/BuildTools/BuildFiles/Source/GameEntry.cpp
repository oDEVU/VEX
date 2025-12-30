#include <cr.h>
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include <VexBuildVersion.hpp>

#ifndef VEX_MAIN_SCENE
#define VEX_MAIN_SCENE "Assets/scenes/main.json"
#endif
#ifndef VEX_WINDOW_MODE
#define VEX_WINDOW_MODE 0
#endif
#ifndef VEX_RESOLUTION_MODE
#define VEX_RESOLUTION_MODE 0
#endif
#ifdef VEX_VSYNC
    #define ON 1
    #define OFF 0
    #define true 1
    #define false 0

    #if VEX_VSYNC
        #undef VEX_VSYNC
        #define VEX_VSYNC true
    #else
        #undef VEX_VSYNC
        #define VEX_VSYNC false
    #endif

    #undef ON
    #undef OFF
    #undef true
    #undef false
#endif

#ifndef VEX_VSYNC
    #define VEX_VSYNC true
#endif

#if defined(_WIN32) && defined(_DEBUG)
#include <Jolt/Jolt.h>

namespace JPH {
    static bool DummyAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine) {
        return true;
    }

    AssertFailedFunction AssertFailed = DummyAssertFailed;
}
#endif

extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    const char* VexModule_GetExpectedHash() {
        return VEX_ENGINE_BUILD_ID;
    }
}

extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #else
    __attribute__((visibility("default")))
    #endif
    void VexGame_Init(vex::Engine* engine) {
        if (!engine) return;

        if (!engine->getSceneManager()) {
            vex::throw_error("SceneManager is NULL!");
        }

        int winMode = VEX_WINDOW_MODE;
        if (winMode == 1) engine->setFullscreen(true, false);
        else if (winMode == 2) engine->setFullscreen(true, true);
        else engine->setFullscreen(false);

        engine->setResolutionMode(static_cast<vex::ResolutionMode>(VEX_RESOLUTION_MODE));

        engine->setVSync(VEX_VSYNC);

        engine->getSceneManager()->loadScene(VEX_MAIN_SCENE, *engine);
    }
}

#ifdef __linux__
extern "C" __attribute__((visibility("default"))) int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
#else
extern "C" __declspec(dllexport) int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
#endif

    vex::Engine* engine = (vex::Engine*)ctx->userdata;

    if (!engine) {
        vex::log(vex::LogLevel::CRITICAL, "Engine pointer is NULL!");
        fflush(stdout);
        return -1;
    }

    static int frameLimit = -1;

    switch (operation) {
        case CR_LOAD:
                    try {
                        vex::log(vex::LogLevel::INFO, "[DEBUG] Entering CR_LOAD...");
                        fflush(stdout);

                        if (!engine->getSceneManager()) {
                            vex::throw_error("SceneManager is NULL!");
                        }

                        vex::log(vex::LogLevel::INFO, "[DEBUG] SceneManager pointer valid: %p", (void*)engine->getSceneManager());

                        if (ctx->version == 1) {
                            int winMode = VEX_WINDOW_MODE;
                            if (winMode == 1) engine->setFullscreen(true, false);
                            else if (winMode == 2) engine->setFullscreen(true, true);
                            else engine->setFullscreen(false);

                            engine->setResolutionMode(static_cast<vex::ResolutionMode>(VEX_RESOLUTION_MODE));

                            engine->setVSync(VEX_VSYNC);

                            engine->getSceneManager()->loadScene(VEX_MAIN_SCENE, *engine);
                        } else {
                            if(!engine->getLastLoadedScenes().empty()){
                                for (const auto& sceneName : engine->getLastLoadedScenes()) {
                                    vex::log(vex::LogLevel::INFO, "[DEBUG] Reloading scene: %s", sceneName.c_str());
                                    engine->getSceneManager()->loadScene(sceneName, *engine);
                                }
                            } else {
                                vex::log(vex::LogLevel::INFO, "[DEBUG] Loaded scenes empty, loading default.");
                                engine->getSceneManager()->loadScene(VEX_MAIN_SCENE, *engine);
                            }
                        }

                        if (frameLimit != -1) {
                            engine->setFrameLimit(frameLimit);
                        }
                    } catch (const std::exception& e) {
                        vex::log(vex::LogLevel::ERROR, "Exception during CR_LOAD");
                        vex::handle_exception(e);
                        return -1;
                    }
                    break;

        case CR_STEP:
            break;

        case CR_UNLOAD:
            try {
                vex::log(vex::LogLevel::INFO, "[DEBUG] Unloading...");
                frameLimit = engine->getFrameLimit();
                engine->setFrameLimit(1);
                fflush(stdout);
                engine->WaitForGpu();
                engine->prepareScenesForHotReload();
                engine->getSceneManager()->clearScenes();
                vex::ComponentRegistry::getInstance().clearDynamicComponents();
                vex::GameObjectFactory::getInstance().clearDynamicGameObjects();
                vex::log(vex::LogLevel::INFO, "[HotReload] Unload Cleanup Finished.");
                fflush(stdout);
            } catch (const std::exception& e) {
                vex::log(vex::LogLevel::ERROR, "Exception during CR_UNLOAD");
                vex::handle_exception(e);
            }
            break;

        case CR_CLOSE:
            return 0;
            break;
    }

    return 0;
}
