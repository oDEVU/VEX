#include <cr.h>
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

#ifndef VEX_MAIN_SCENE
#define VEX_MAIN_SCENE "Assets/scenes/main.json"
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
                fflush(stdout);
                engine->prepareScenesForHotReload();
                engine->getSceneManager()->clearScenes();
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
