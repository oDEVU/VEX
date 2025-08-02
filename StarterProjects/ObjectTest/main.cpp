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

class ObjectTest : public vex::Engine {
public:
    using vex::Engine::Engine; // Inherit constructor
    bool animate = true;

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

    vex::ModelObject* humanEntity1;
    vex::ModelObject* humanEntity2;
    vex::ModelObject* humanEntity3;
    vex::ModelObject* humanEntity4;
    vex::ModelObject* viperEntity1;
    vex::ModelObject* viperEntity2;
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

        vex::MeshComponent humanMesh = vex::createMeshFromPath("Assets/human.obj", *this);
        vex::TransformComponent humanTransform = vex::TransformComponent{
            glm::vec3{0.0f, 0.3f, 0.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{1.0f, 1.0f, 1.0f}};
        humanEntity1 = vex::createModelFromComponents("human1", humanMesh, humanTransform, *this);
        humanTransform.position += glm::vec3(5,0,0);
        humanEntity2 = vex::createModelFromComponents("human2", humanMesh, humanTransform, *this);
        humanTransform.position += glm::vec3(5,0,0);
        humanEntity3 = vex::createModelFromComponents("human3", humanMesh, humanTransform, *this);
        humanTransform.position += glm::vec3(5,0,0);
        humanEntity4 = vex::createModelFromComponents("human4", humanMesh, humanTransform, *this);

        humanEntity2->ParentTo(humanEntity1->GetEntity());
        humanEntity3->ParentTo(humanEntity2->GetEntity());
        humanEntity4->ParentTo(humanEntity3->GetEntity());

        viperEntity1 = vex::createModelFromPath("Assets/scene.gltf", "viper1", *this);
        viperEntity2 = vex::createModelFromPath("Assets/scene.gltf", "viper2", *this);
        viperEntity2->GetComponent<vex::TransformComponent>().position += glm::vec3{2.0f, 0.0f, 0.0f};
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
        //pass
        if (humanEntity1 && humanEntity1->isValid()) {
            humanEntity1->GetComponent<vex::TransformComponent>().rotation.y = m_frame;
        }

        // testing if i can remove entities at runtime
        if (m_frame == 120000 && humanEntity1 && humanEntity1->isValid()) {
            delete humanEntity1;
            humanEntity1 = nullptr;
        }
        if (m_frame == 240000 && humanEntity2 && humanEntity2->isValid()) {
            delete humanEntity2;
            humanEntity2 = nullptr;
        }
        if (m_frame == 360000 && humanEntity3 && humanEntity3->isValid()) {
            delete humanEntity3;
            humanEntity3 = nullptr;
        }
        if (m_frame == 480000 && humanEntity4 && humanEntity4->isValid()) {
            delete humanEntity4;
            humanEntity4 = nullptr;
        }
        if (m_frame == 160000 && viperEntity1 && viperEntity1->isValid()) {
            delete viperEntity1;
            viperEntity1 = nullptr;
        }
        if (m_frame == 320000 && viperEntity2 && viperEntity2->isValid()) {
            delete viperEntity2;
            viperEntity2 = nullptr;
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

        vex::GameInfo gInfo{"ObjectTest",
                            0,
                            1,
                            0
        };
        ObjectTest game("ObjectTest", 1280, 720, gInfo);

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
