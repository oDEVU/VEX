#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/UiComponent.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include <memory>

namespace vex {
class Player : public GameObject {
public:
    Player(Engine& engine, const std::string& name) : GameObject(engine, name) {
        AddComponent(TransformComponent{});
        AddComponent(UiComponent{engine.createVexUI()});
    }

    void BeginPlay() override {
        log("Player \"%s\" started", GetComponent<NameComponent>().name.c_str());

        //GetComponent<UiComponent>().m_vexUI->init();
        GetComponent<UiComponent>().m_vexUI->load("Assets/ui/example.json");
        GetComponent<UiComponent>().m_vexUI->setOnClick("pause", []() {
            log("Pause button clicked");
        });

        //Camera = new vex::CameraObject(
        //    m_engine,
        //    "mainCamera"
            //);
        //Camera -> ParentTo(this->GetEntity());

        // Action Inputs need deltaTime
        vex::InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().position += GetComponent<vex::TransformComponent>().getForwardVector(m_engine.getRegistry()) * (deltaTime * 2);
        });
        inputComp.addBinding(SDL_SCANCODE_S, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().position -= GetComponent<vex::TransformComponent>().getForwardVector(m_engine.getRegistry()) * (deltaTime * 2);
        });
        inputComp.addBinding(SDL_SCANCODE_D, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().position += GetComponent<vex::TransformComponent>().getRightVector(m_engine.getRegistry()) * (deltaTime * 2);
        });
        inputComp.addBinding(SDL_SCANCODE_A, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().position -= GetComponent<vex::TransformComponent>().getRightVector(m_engine.getRegistry()) * (deltaTime * 2);
        });

        inputComp.addBinding(SDL_SCANCODE_E, vex::InputActionState::Pressed, [this](float) {
            if(m_engine.getInputMode() == vex::InputMode::Game){
                m_engine.setInputMode(vex::InputMode::UI);
            }else{
                m_engine.setInputMode(vex::InputMode::Game);
            }
        });

        inputComp.addBinding(SDL_SCANCODE_F, vex::InputActionState::Pressed, [this](float) {
            GetComponent<UiComponent>().visible = !GetComponent<UiComponent>().visible;
        });

        // Axis inputs dont need delta time since movement beetween frames will be proportionaly smaller
        inputComp.addMouseAxisBinding(vex::MouseAxis::X, [this](float axis) {
            GetComponent<vex::TransformComponent>().rotation.y += axis * 0.05;
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis) {
            GetComponent<vex::TransformComponent>().rotation.x -= axis * 0.05;
        });

        AddComponent(inputComp);
    }

    void Update(float deltaTime) override {
        GetComponent<UiComponent>().m_vexUI->setText("fps", "FPS: " + std::to_string((int)(1.f/deltaTime)));
        // Example behavior
        // auto& transform = GetComponent<TransformComponent>();
        // transform.position.x += deltaTime; // Move right
        //log("Player transform : %f,%f,%f ", GetComponent<TransformComponent>().position.x, GetComponent<TransformComponent>().position.y, GetComponent<TransformComponent>().position.z);
        //log("Player camera transform : %f,%f,%f ", Camera->GetComponent<TransformComponent>().position.x, Camera->GetComponent<TransformComponent>().position.y, Camera->GetComponent<TransformComponent>().position.z);
    }
private:
//vex::CameraObject* Camera;
};
}
