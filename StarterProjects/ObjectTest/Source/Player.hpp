#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/UiComponent.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/PhysicsSystem.hpp"
#include "components/SceneManager.hpp"
#include <cmath>
#include <memory>

namespace vex {
class Player : public GameObject {
public:
    Player(Engine& engine, const std::string& name) : GameObject(engine, name) {
        AddComponent(TransformComponent{engine.getRegistry()});
        AddComponent(UiComponent{engine.createVexUI()});
    }

    void BeginPlay() override {
        log("Player \"%s\" started", GetComponent<NameComponent>().name.c_str());

        resumeGame();

        vex::InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getForwardVector() * (deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_S, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getForwardVector() * -(deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_D, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getRightVector() * (deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_A, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getRightVector() * -(deltaTime * 2));
        });

        // Input Mode Toggle
        inputComp.addBinding(SDL_SCANCODE_E, vex::InputActionState::Pressed, [this](float) {
            if(m_engine.getInputMode() == vex::InputMode::Game){
                m_engine.setInputMode(vex::InputMode::UI);
            }else{
                m_engine.setInputMode(vex::InputMode::Game);
            }
        });

        // UI Visibility Toggle
        inputComp.addBinding(SDL_SCANCODE_F, vex::InputActionState::Pressed, [this](float) {
            GetComponent<UiComponent>().visible = !GetComponent<UiComponent>().visible;
        });

        // Pause
        inputComp.addBinding(SDL_SCANCODE_P, vex::InputActionState::Pressed, [this](float) {
            pauseGame();
        });

        // Resolution switching
        inputComp.addBinding(SDL_SCANCODE_1, vex::InputActionState::Pressed, [this](float) { GetEngine().setResolutionMode(vex::ResolutionMode::NATIVE); });
        inputComp.addBinding(SDL_SCANCODE_2, vex::InputActionState::Pressed, [this](float) { GetEngine().setResolutionMode(vex::ResolutionMode::PS1_SHARP); });
        inputComp.addBinding(SDL_SCANCODE_3, vex::InputActionState::Pressed, [this](float) { GetEngine().setResolutionMode(vex::ResolutionMode::RES_240P); });
        inputComp.addBinding(SDL_SCANCODE_4, vex::InputActionState::Pressed, [this](float) { GetEngine().setResolutionMode(vex::ResolutionMode::RES_480P); });

        inputComp.addMouseAxisBinding(vex::MouseAxis::X, [this](float axis) {
            GetComponent<vex::TransformComponent>().addYaw(-axis * 0.05);
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis) {
            GetComponent<vex::TransformComponent>().addPitch(-axis * 0.05);
        });

        AddComponent(inputComp);
    }

    void pauseGame() {
        std::shared_ptr<VexUI> ui = GetComponent<UiComponent>().m_vexUI;
        ui->load("Assets/ui/pause.json");

        // TEST: The resume button is rotated 45 degrees in pause.json.
        // Clicking this verifies the 'findWidgetAt' rotation math.
        ui->setOnClick("resume", [this]() {
            this->resumeGame();
        });

        m_engine.setInputMode(vex::InputMode::UI);
        m_engine.setPaused(true);
    }

    void resumeGame() {
        std::shared_ptr<VexUI> ui = GetComponent<UiComponent>().m_vexUI;
        ui->load("Assets/ui/example.json");

        ui->setOnClick("pause", [this]() {
            this->pauseGame();
        });

        // TEST: Verify we can click the logo even though it is spinning/scaling in Update()
        ui->setOnClick("logo", []() {
            log("CLICKED THE SPINNING LOGO!");
        });

        m_engine.setInputMode(vex::InputMode::Game);
        m_engine.setPaused(false);
    }

    void Update(float deltaTime) override {
        // Only animate if not paused
        if (m_engine.isPaused()) return;

        m_totalTime += deltaTime;
        std::shared_ptr<VexUI> ui = GetComponent<UiComponent>().m_vexUI;

        // --- TEST 1: Runtime Text & Color ---
        float fps = 1.f / deltaTime;
        ui->setText("fps", "FPS: " + std::to_string((int)fps));

        // Change color based on FPS (Runtime Style Change)
        if (fps > 55.0f)
            ui->setColor("fps", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
        else if (fps < 30.0f)
            ui->setColor("fps", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
        else
            ui->setColor("fps", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow


        // --- TEST 2: Runtime Rotation & Sizing (Animation) ---
        // Find the logo and animate it
        if (ui->getWidget("logo")) {
            // Spin back and forth 15 degrees
            float rotation = std::sin(m_totalTime * 2.0f) * 15.0f;
            ui->setRotation("logo", rotation);

            // Pulse size between 100% and 120%
            float scale = 1.0f + (std::sin(m_totalTime * 4.0f) * 0.2f);
            // Original size in json is [96, 64]
            ui->setSize("logo", 96.0f * scale, 64.0f * scale);
        }

        // Rotate a generic object in the scene (existing code)
        GameObject* object = m_engine.getSceneManager()->GetScene("Assets/scenes/main.json")->GetAllGameObjectsByName("MyModel")[0];
        if (object) {
            object->GetComponent<TransformComponent>().addPitch(0.1f);
        }
    }

private:
    float m_totalTime = 0.0f;
};
}
