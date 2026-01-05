#include "Player.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

namespace vex {

    Player::Player(Engine& engine, const std::string& name) : GameObject(engine, name) {
        AddComponent(TransformComponent{});
        AddComponent(CharacterComponent{});
        camera = new CameraObject(engine, "PlayerCamera");
        camera->ParentTo(GetEntity());
        camera->GetComponent<TransformComponent>().setLocalPosition(glm::vec3(0.0f, 1.0f, 0.0f));
        camera->GetComponent<CameraComponent>().fov = 90.f;
    }

    void Player::BeginPlay() {
        // basic movement

        InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_W, InputActionState::Held, [this](float deltaTime) {
            auto& cc = GetComponent<CharacterComponent>();
            glm::vec3 fwd = GetComponent<TransformComponent>().getForwardVector();
            cc.controlInput += fwd * movementSpeed;
        });
        inputComp.addBinding(SDL_SCANCODE_S, InputActionState::Held, [this](float deltaTime) {
            auto& cc = GetComponent<CharacterComponent>();
            glm::vec3 fwd = GetComponent<TransformComponent>().getForwardVector();
            cc.controlInput -= fwd * movementSpeed;
        });
        inputComp.addBinding(SDL_SCANCODE_D, InputActionState::Held, [this](float deltaTime) {
            auto& cc = GetComponent<CharacterComponent>();
            glm::vec3 right = GetComponent<TransformComponent>().getRightVector();
            cc.controlInput += right * movementSpeed;
        });
        inputComp.addBinding(SDL_SCANCODE_A, InputActionState::Held, [this](float deltaTime) {
            auto& cc = GetComponent<CharacterComponent>();
            glm::vec3 right = GetComponent<TransformComponent>().getRightVector();
            cc.controlInput -= right * movementSpeed;
        });
        inputComp.addBinding(SDL_SCANCODE_SPACE, InputActionState::Pressed, [this](float deltaTime) {
            auto& cc = GetComponent<CharacterComponent>();
            if(cc.isSupported()){
                cc.controlInput += glm::vec3(0.0f, 10.f, 0.0f);
            }
        });

        inputComp.addMouseAxisBinding(MouseAxis::X, [this](float axis) {
            GetComponent<TransformComponent>().addYaw(-axis * 0.05);
        });
        inputComp.addMouseAxisBinding(MouseAxis::Y, [this](float axis) {
            if(camera){
                camera->GetComponent<TransformComponent>().addPitch(axis * -0.05);
            }
        });

        AddComponent(inputComp);
    }

    void Player::Update(float deltaTime) {
    }

    REGISTER_GAME_OBJECT(Player);
}
