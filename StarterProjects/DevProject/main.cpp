// CoreEngine
#include "Engine.hpp"
#include "components/GameInfo.hpp"
#include "components/errorUtils.hpp"
#include "components/ResolutionManager.hpp"
#include "components/InputSystem.hpp"

// Object System
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/InputComponent.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameObjects/CameraObject.hpp"

#include "glm/fwd.hpp"

#include <cstdlib>
#include <memory>
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
        }
    }

    vex::ModelObject* viper;
    vex::ModelObject* penguin;
    vex::ModelObject* cube;
    vex::ModelObject* cube2;
    vex::ModelObject* sphere;
    vex::ModelObject* smoothsphere;
    vex::CameraObject* Camera;


    void beginGame() override {
        Camera = new vex::CameraObject(
            *this,
            "mainCamera",
            vex::TransformComponent{glm::vec3{0,0,5}, glm::vec3{0,270,0}, glm::vec3{1,1,1}},
            vex::CameraComponent{45.0f, 0.1f, 100.0f}
        );

        vex::InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, vex::InputActionState::Held, [this](float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().position += Camera->GetComponent<vex::TransformComponent>().getForwardVector() * deltaTime;
        });
        inputComp.addBinding(SDL_SCANCODE_S, vex::InputActionState::Held, [this](float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().position -= Camera->GetComponent<vex::TransformComponent>().getForwardVector() * deltaTime;
        });
        inputComp.addBinding(SDL_SCANCODE_D, vex::InputActionState::Held, [this](float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().position += Camera->GetComponent<vex::TransformComponent>().getRightVector() * deltaTime;
        });
        inputComp.addBinding(SDL_SCANCODE_A, vex::InputActionState::Held, [this](float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().position -= Camera->GetComponent<vex::TransformComponent>().getRightVector() * deltaTime;
        });

        inputComp.addBinding(SDL_SCANCODE_E, vex::InputActionState::Pressed, [this](float) {
            if(getInputMode() == vex::InputMode::Game){
                setInputMode(vex::InputMode::UI);
            }else{
                setInputMode(vex::InputMode::Game);
            }
        });

        inputComp.addMouseAxisBinding(vex::MouseAxis::X, [this](float axis, float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().rotation.y += axis * deltaTime * 25;
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis, float deltaTime) {
            Camera->GetComponent<vex::TransformComponent>().rotation.x -= axis * deltaTime * 25;
        });

        Camera->AddComponent(inputComp);
        // Load first models
        log("BeginGame: Trying to load models...");
        vex::MeshComponent cubeMesh = vex::createMeshFromPath("Assets/cube.obj", *this);
        cube = vex::createModelFromComponents("cube", cubeMesh, vex::TransformComponent(), *this);
        vex::MeshComponent cubeMesh2 = vex::createMeshFromPath("Assets/untitled.obj", *this);
        cube2 = vex::createModelFromComponents("cube2", cubeMesh2, vex::TransformComponent(), *this);
        cube2 -> GetComponent<vex::TransformComponent>().position = glm::vec3{3,0,0};
        vex::MeshComponent sphereMesh = vex::createMeshFromPath("Assets/SimpleSphere.fbx", *this);
        sphere = vex::createModelFromComponents("sphere", sphereMesh, vex::TransformComponent(), *this);
        sphere -> GetComponent<vex::TransformComponent>().position = glm::vec3{-3,0,0};
        vex::MeshComponent sphereMesh2 = vex::createMeshFromPath("Assets/SimpleSmoothSphere.obj", *this);
        smoothsphere = vex::createModelFromComponents("smoothsphere", sphereMesh2, vex::TransformComponent(), *this);
        smoothsphere -> GetComponent<vex::TransformComponent>().position = glm::vec3{-5,0,0};
        smoothsphere -> GetComponent<vex::TransformComponent>().rotation = glm::vec3{0,90,0};
        //loadModel("Assets/human.obj", "human");
        vex::MeshComponent viperMesh = vex::createMeshFromPath("Assets/scene.gltf", *this);
        viper = vex::createModelFromComponents("viper", viperMesh, vex::TransformComponent(), *this);

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
        if(m_frame == 10000){
            vex::MeshComponent penguinMesh = vex::createMeshFromPath("Assets/PenguinBaseMesh.obj", *this);
            penguin = vex::createModelFromComponents("penguin", penguinMesh, vex::TransformComponent(), *this);
        }

        if(animate){
                    //Debuging camera rotations
                    //getCamera()->transform.rotation = glm::vec3{0,float(m_frame)/1000,0};
                    //log("Camera rot: %f", float(m_frame)/1000);

                    if (viper) {
                        viper->GetComponent<vex::TransformComponent>().rotation.y = (float(m_frame)/100);
                    }

                    if (penguin && penguin->isValid() && m_frame > 100) {
                        penguin->GetComponent<vex::TransformComponent>().position.y = (sin(float(m_frame)/100)/2)+1;
                    }

                    if (cube) {
                        cube->GetComponent<vex::TransformComponent>().scale = glm::vec3{sin(float(m_frame)/1000), sin(float(m_frame)/1000), sin(float(m_frame)/1000)};
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
