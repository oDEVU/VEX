#pragma once
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

namespace vex {
class Player : public GameObject {
public:
    Player(Engine& engine, const std::string& name) : GameObject(engine, name) {
        // Example: Add a transform component
        AddComponent(TransformComponent{glm::vec3{0.0f, 0.0f, 0.0f}});
    }

    void BeginPlay() override {
        log("Player \"%s\" started", GetComponent<NameComponent>().name.c_str());


        Camera = new vex::CameraObject(
            m_engine,
            "mainCamera",
            vex::TransformComponent{glm::vec3{0,0,5}, glm::vec3{0,270,0}, glm::vec3{1,1,1}},
            vex::CameraComponent{45.0f, 0.1f, 100.0f}
        );

        // Action Inputs need deltaTime
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
            if(m_engine.getInputMode() == vex::InputMode::Game){
                m_engine.setInputMode(vex::InputMode::UI);
            }else{
                m_engine.setInputMode(vex::InputMode::Game);
            }
        });

        // Axis inputs dont need delta time since movement beetween frames will be proportionaly smaller
        inputComp.addMouseAxisBinding(vex::MouseAxis::X, [this](float axis) {
            Camera->GetComponent<vex::TransformComponent>().rotation.y += axis * 0.05;
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis) {
            Camera->GetComponent<vex::TransformComponent>().rotation.x -= axis * 0.05;
        });

        Camera->AddComponent(inputComp);
    }

    void Update(float deltaTime) override {
        // Example behavior
        // auto& transform = GetComponent<TransformComponent>();
        // transform.position.x += deltaTime; // Move right
        //log("Player: %s, update called", GetComponent<NameComponent>().name.c_str());
    }
private:
vex::CameraObject* Camera;
};
}
