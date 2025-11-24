#include <cr.h>
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/SceneManager.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

extern "C" {
    __attribute__((visibility("default")))
    int cr_main(struct cr_plugin *ctx, enum cr_op operation) {

    vex::Engine* engine = (vex::Engine*)ctx->userdata;

    if (!engine) {
                vex::log("[DEBUG] CRITICAL: Engine pointer is NULL!");
                //fflush(stdout);
                return -1;
            }

    switch (operation) {
        case CR_LOAD:
        vex::log("[DEBUG] Entering CR_LOAD...");
            if (engine->getSceneManager()) {
                                vex::log("[DEBUG] SceneManager pointer seems valid: %p", (void*)engine->getSceneManager());
                            } else {
                                vex::log("[DEBUG] CRITICAL: SceneManager is NULL!\n");
                            }

            if (ctx->version == 1) {
                engine->getSceneManager()->loadScene("Assets/scenes/main.json", *engine);
            } else {
                if(!engine->getLastLoadedScenes().empty()){
                    for (const auto& sceneName : engine->getLastLoadedScenes()) {
                        vex::log("[DEBUG] Reloading scene: %s", sceneName.c_str());
                        //fflush(stdout);
                        engine->getSceneManager()->loadScene(sceneName, *engine);
                    }
                }else{
                    vex::log("[DEBUG] Loaded scenes are empty");
                    //fflush(stdout);
                    engine->getSceneManager()->loadScene("Assets/scenes/main.json", *engine);
                }
            }
            break;

        case CR_STEP:
            break;

        case CR_UNLOAD:
            vex::log("[DEBUG] Unloading...");
            engine->prepareScenesForHotReload();
            engine->getSceneManager()->clearScenes();
            //engine->getRegistry().clear();
            break;

        case CR_CLOSE:
            return 0;
            break;
    }

    return 0;
}
}
