#include "Engine.hpp"
#include "components/Model.hpp"

#include <cstdlib>
#include <sys/types.h>

class MyGame : public vex::Engine {
public:
    using vex::Engine::Engine; // Inherit constructor
    bool animate = true;

    void processEvent(const SDL_Event& event) {
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
        }
    }

    bool modelsLoaded = false;
    vex::Model* viper;
    vex::Model* penguin;
    vex::Model* cube;

    void update(float deltaTime) override {
        // Custom game logic
        if(!modelsLoaded){
            SDL_Log("Main loop: Trying to load models...");
            loadModel("assets/cube.obj", "cube");
            cube = getModel("cube");
            //loadModel("assets/human.obj", "human");
            loadModel("assets/PenguinBaseMesh.obj", "penguin");
            loadModel("assets/scene.gltf", "viper");
            viper = getModel("viper");
            modelsLoaded = true;
        }

        if(frame == 100){
            penguin = getModel("penguin");
        }

        if(animate){

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
        MyGame game("My Game", 1280, 720);
        SDL_Log("Game instance created successfully");

        SDL_Log("Starting game loop...");
        game.run();
        SDL_Log("Game loop exited normally");
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game error: %s", e.what());
        return 1;
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown error occurred");
        return 1;
    }

    SDL_Log("Exiting cleanly");
    return 0;
}
