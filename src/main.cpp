#include "components/engine.hpp"

#include <cstdlib>
#include <sys/types.h>

class MyGame : public vex::Engine {
public:
    using vex::Engine::Engine; // Inherit constructor

    u_int64_t frame = 0;

    void processEvent(const SDL_Event& event) {
        if (event.type == SDL_EVENT_KEY_DOWN) {
            const SDL_KeyboardEvent& key_event = event.key;

            // Physical key position (recommended for games)
            if (key_event.scancode == SDL_SCANCODE_ESCAPE) {
                m_running = false;
            }

            // Virtual key (respects keyboard layout)
            if (key_event.key == SDLK_ESCAPE) {
                m_running = false;
            }
        }
    }

    void update(float deltaTime) override {
        // Custom game logic
        if(frame == 5){
            SDL_Log("Main loop: Trying to load models...");
            m_interface->loadModel("assets/cube.obj", "cube");
            //m_interface->loadModel("assets/human.obj", "human");
            m_interface->loadModel("assets/PenguinBaseMesh.obj", "penguin");
            m_interface->loadModel("assets/scene.gltf", "viper");
        }

        frame++;
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
