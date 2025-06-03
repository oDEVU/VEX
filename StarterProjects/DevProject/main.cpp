#include "Engine.hpp"
#include "components/GameInfo.hpp"
#include "components/Model.hpp"
#include "components/Camera.hpp"
#include "components/errorUtils.hpp"

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

            //SDL_Log("delta: %f", deltaTime);
        }
    }

    vex::Model* viper;
    vex::Model* penguin;
    vex::Model* cube;

    void beginGame() override {
        // Load first models
        SDL_Log("BeginGame: Trying to load models...");
        loadModel("Assets/cube.obj", "cube");
        cube = getModel("cube");
        //loadModel("Assets/human.obj", "human");
        loadModel("Assets/PenguinBaseMesh.obj", "penguin");
        loadModel("Assets/scene.gltf", "viper");
        viper = getModel("viper");

        // setup camera transforms
        SDL_Log("BeginGame: CameraSetup ..");
        getCamera()->transform.position = glm::vec3{0,0,5};
        getCamera()->transform.rotation = glm::vec3{0,270,0};
    }

    void update(float deltaTime) override {

        // load penguin at runtime
        if(frame == 100){
            penguin = getModel("penguin");
        }

        if(animate){
                    //Debuging camera rotations
                    //getCamera()->transform.rotation = glm::vec3{0,float(frame)/1000,0};
                    //SDL_Log("Camera rot: %f", float(frame)/1000);

                    if (viper) {
                        viper->transform.rotation.y = (float(frame)/100);
                    }

                    if (penguin && frame > 100) {
                        penguin->transform.position.y = (sin(float(frame)/100)/2)+1;
                    }

                    if (cube) {
                        cube->transform.scale = glm::vec3{sin(float(frame)/1000), sin(float(frame)/1000), sin(float(frame)/1000)};
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
        SDL_Log("Creating game instance...");

        vex::GameInfo gInfo{"DevProject",
                            0,
                            1,
                            0
        };
        DevProject game("DevProject", 1280, 720, gInfo);

        SDL_Log("Game instance created successfully");
        SDL_Log("Starting game loop...");
        game.run();
        SDL_Log("Game loop exited normally");
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game error:");
        handle_exception(e);
        return 1;
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown error occurred");
        return 1;
    }

    SDL_Log("Exiting cleanly");
    return 0;
}
