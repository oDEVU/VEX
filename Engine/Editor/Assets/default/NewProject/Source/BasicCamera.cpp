#pragma once
#include "BasicCamera.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

namespace vex {

    BasicCamera::BasicCamera(Engine& engine, const std::string& name) : GameObject(engine, name) {
        AddComponent(TransformComponent{});
        AddComponent(CameraComponent{});
    }

    void BasicCamera::BeginPlay() {
        // basic movement

        InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, InputActionState::Held, [this](float deltaTime) {
            GetComponent<TransformComponent>().addLocalPosition(GetComponent<TransformComponent>().getForwardVector() * (deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_S, InputActionState::Held, [this](float deltaTime) {
            GetComponent<TransformComponent>().addLocalPosition(GetComponent<TransformComponent>().getForwardVector() * -(deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_D, InputActionState::Held, [this](float deltaTime) {
            GetComponent<TransformComponent>().addLocalPosition(GetComponent<TransformComponent>().getRightVector() * (deltaTime * 2));
        });
        inputComp.addBinding(SDL_SCANCODE_A, InputActionState::Held, [this](float deltaTime) {
            GetComponent<TransformComponent>().addLocalPosition(GetComponent<TransformComponent>().getRightVector() * -(deltaTime * 2));
        });

        inputComp.addMouseAxisBinding(MouseAxis::X, [this](float axis) {
            GetComponent<TransformComponent>().addYaw(-axis * 0.05);
        });
        inputComp.addMouseAxisBinding(MouseAxis::Y, [this](float axis) {
            GetComponent<TransformComponent>().addPitch(-axis * 0.05);
        });

        AddComponent(inputComp);
    }

    void BasicCamera::Update(float deltaTime) {
        // update
    }

    REGISTER_GAME_OBJECT(BasicCamera);
}
