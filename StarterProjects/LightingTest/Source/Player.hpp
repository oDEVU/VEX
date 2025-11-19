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
        //AddComponent(PhysicsComponent::Sphere(0.5f, vex::BodyType::DYNAMIC));
    }

    void BeginPlay() override {
        log("Player \"%s\" started", GetComponent<NameComponent>().name.c_str());
        GetEngine().setResolutionMode(vex::ResolutionMode::PS1_SHARP);
        GetComponent<UiComponent>().m_vexUI->load("Assets/ui/main.json");

        // Action Inputs need deltaTime
        vex::InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getForwardVector() * (deltaTime * 20));
        });
        inputComp.addBinding(SDL_SCANCODE_S, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getForwardVector() * -(deltaTime * 20));
        });
        inputComp.addBinding(SDL_SCANCODE_D, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getRightVector() * (deltaTime * 20));
        });
        inputComp.addBinding(SDL_SCANCODE_A, vex::InputActionState::Held, [this](float deltaTime) {
            GetComponent<vex::TransformComponent>().addLocalPosition(GetComponent<vex::TransformComponent>().getRightVector() * -(deltaTime * 20));
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
            GetComponent<vex::TransformComponent>().addYaw(-axis * 0.05);
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis) {
            GetComponent<vex::TransformComponent>().addPitch(-axis * 0.05);
        });

        AddComponent(inputComp);
    }

    void Update(float deltaTime) override {
        GetComponent<UiComponent>().m_vexUI->setText("fps", "FPS: " + std::to_string((int)(1.f/deltaTime)));

        glm::vec3 wp = GetComponent<TransformComponent>().getWorldPosition();

        log("pos: %f, %f, %f", wp.x, wp.y, wp.z);
    }
private:
};
}
