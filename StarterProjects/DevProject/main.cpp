#include "Engine.hpp"
#include "components/GameInfo.hpp"
#include "components/Model.hpp"
#include "components/Camera.hpp"
#include "components/errorUtils.hpp"
#include "components/ResolutionManager.hpp"

#include <cstdlib>
#include <sys/types.h>

class DevProject : public vex::Engine {
public:
    using vex::Engine::Engine; // Inherit constructor
    bool animate = true;

    //FIXME: im dumb and this doesnt need delta time.
    void processEvent(const SDL_Event& event, float deltaTime) {
        if (event.type == SDL_EVENT_KEY_DOWN) {
            const SDL_KeyboardEvent& key_event = event.key;

            // Physical key position (recommended for games)
            if (key_event.scancode == SDL_SCANCODE_ESCAPE) {
                m_running = false;
            }

            if (key_event.scancode == SDL_SCANCODE_P) {
                animate = !animate;
            }

            // Virtual key (respects keyboard layout)
            if (key_event.key == SDLK_ESCAPE) {
                m_running = false;
            }

            if (key_event.scancode == SDL_SCANCODE_1) {
                setResolutionMode(vex::ResolutionMode::NATIVE);
            }

            if (key_event.scancode == SDL_SCANCODE_2) {
                setResolutionMode(vex::ResolutionMode::PS1_SHARP);
            }

            if (key_event.scancode == SDL_SCANCODE_3) {
                setResolutionMode(vex::ResolutionMode::RES_240P);
            }

            if (key_event.scancode == SDL_SCANCODE_4) {
                setResolutionMode(vex::ResolutionMode::RES_480P);
            }

            // Basic camera input, just for testing, do not do it like this lmao

            if (key_event.scancode == SDL_SCANCODE_W) {
                getCamera()->transform.position = getCamera()->transform.position + (getCamera()->transform.getForwardVector()*float(deltaTime*250));
            }

            if (key_event.scancode == SDL_SCANCODE_S) {
                getCamera()->transform.position = getCamera()->transform.position + (getCamera()->transform.getForwardVector()*float(deltaTime*-250));
            }

            if (key_event.scancode == SDL_SCANCODE_D) {
                getCamera()->transform.position = getCamera()->transform.position + (getCamera()->transform.getRightVector()*float(deltaTime*250));
            }

            if (key_event.scancode == SDL_SCANCODE_A) {
                getCamera()->transform.position = getCamera()->transform.position + (getCamera()->transform.getRightVector()*float(deltaTime*-250));
            }

            //log("delta: %f", deltaTime);
        }
    }

    vex::Model* viper;
    vex::Model* penguin;
    vex::Model* cube;
    vex::Model* cube2;
    vex::Model* sphere;
    vex::Model* smoothsphere;


    void beginGame() override {
        // Load first models
        log("BeginGame: Trying to load models...");
        loadModel("Assets/cube.obj", "cube");
        cube = getModel("cube");
        loadModel("Assets/wierd_textured_cube/untitled.obj", "cube2");
        cube2 = getModel("cube2");
        cube2 -> transform.position = glm::vec3{3,0,0};
        loadModel("Assets/SimpleSphere.fbx", "sphere");
        sphere = getModel("sphere");
        sphere -> transform.position = glm::vec3{-3,0,0};
        loadModel("Assets/SimpleSmoothSphere.obj", "smoothsphere");
        smoothsphere = getModel("smoothsphere");
        smoothsphere -> transform.position = glm::vec3{-5,0,0};
        smoothsphere -> transform.rotation = glm::vec3{0,90,0};
        //loadModel("Assets/human.obj", "human");
        loadModel("Assets/PenguinBaseMesh.obj", "penguin");
        loadModel("Assets/scene.gltf", "viper");
        viper = getModel("viper");

        // setup camera transforms
        log("BeginGame: CameraSetup ..");
        getCamera()->transform.position = glm::vec3{0,0,5};
        getCamera()->transform.rotation = glm::vec3{0,270,0};

#if DEBUG
        m_imgui->addUIFunction([this]() {
            ImGui::Begin("Engine Stats");
            ImGui::Text("Renderer: Vulkan");
            ImGui::Text("Frame: %i", m_frame);
            ImGui::End();
        });
#endif
    }

    void update(float deltaTime) override {

        // load penguin at runtime
        if(m_frame == 100){
            penguin = getModel("penguin");
        }

        if(animate){
                    //Debuging camera rotations
                    //getCamera()->transform.rotation = glm::vec3{0,float(m_frame)/1000,0};
                    //log("Camera rot: %f", float(m_frame)/1000);

                    if (viper) {
                        viper->transform.rotation.y = (float(m_frame)/100);
                    }

                    if (penguin && m_frame > 100) {
                        penguin->transform.position.y = (sin(float(m_frame)/100)/2)+1;
                    }

                    if (cube) {
                        cube->transform.scale = glm::vec3{sin(float(m_frame)/1000), sin(float(m_frame)/1000), sin(float(m_frame)/1000)};
                    }
        }
    }

    void render() override {
        // Custom rendering if needed
        Engine::render(); // Call base render
    }
};

int main(int argc, char* argv[]) {
    try {
        log("Creating game instance...");

        vex::GameInfo gInfo{"DevProject",
                            0,
                            1,
                            0
        };
        DevProject game("DevProject", 1280, 720, gInfo);

        log("Game instance created successfully");
        log("Starting game loop...");
        game.run();
        log("Game loop exited normally");
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game error:");
        handle_exception(e);
        return 1;
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown error occurred");
        return 1;
    }

    log("Exiting cleanly");
    return 0;
}
