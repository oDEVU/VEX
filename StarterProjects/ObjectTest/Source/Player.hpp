#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/UiComponent.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/SceneManager.hpp"
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

        //GetComponent<UiComponent>().m_vexUI->init();
        resumeGame();

        //Camera = new vex::CameraObject(
        //    m_engine,
        //    "mainCamera"
            //);
        //Camera -> ParentTo(this->GetEntity());

        // Action Inputs need deltaTime
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

        inputComp.addBinding(SDL_SCANCODE_P, vex::InputActionState::Pressed, [this](float) {
            pauseGame();
        });

        // Axis inputs dont need delta time since movement beetween frames will be proportionaly smaller
        inputComp.addMouseAxisBinding(vex::MouseAxis::X, [this](float axis) {
            GetComponent<vex::TransformComponent>().addYaw(axis * 0.05);
        });
        inputComp.addMouseAxisBinding(vex::MouseAxis::Y, [this](float axis) {
            // dirty fix to fix camera fliping
            if(GetComponent<vex::TransformComponent>().getWorldRotation().x < 90 && GetComponent<vex::TransformComponent>().getWorldRotation().x > -90){
                GetComponent<vex::TransformComponent>().addPitch(-axis * 0.05);
            }

            if(GetComponent<vex::TransformComponent>().getWorldRotation().x >= 90){
                GetComponent<vex::TransformComponent>().setWorldRotation({89.9, GetComponent<vex::TransformComponent>().getWorldRotation().y, GetComponent<vex::TransformComponent>().getWorldRotation().z});
            }else if(GetComponent<vex::TransformComponent>().getWorldRotation().x <= -90){
                GetComponent<vex::TransformComponent>().setWorldRotation({-89.9, GetComponent<vex::TransformComponent>().getWorldRotation().y, GetComponent<vex::TransformComponent>().getWorldRotation().z});
            }

            //log("Camera rotation: %f, %f, %f", GetComponent<vex::TransformComponent>().getWorldRotation().x, GetComponent<vex::TransformComponent>().getWorldRotation().y, GetComponent<vex::TransformComponent>().getWorldRotation().z);
        });

        AddComponent(inputComp);
    }

    void pauseGame() {
        std::shared_ptr<VexUI> ui = GetComponent<UiComponent>().m_vexUI;
        ui->load("Assets/ui/pause.json");
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
        m_engine.setInputMode(vex::InputMode::Game);
        m_engine.setPaused(false);
    }

    void Update(float deltaTime) override {
        GetComponent<UiComponent>().m_vexUI->setText("fps", "FPS: " + std::to_string((int)(1.f/deltaTime)));

        // temponary just to test stuff, dont do that, dont retrive object everyframe.
        std::shared_ptr<GameObject> object = m_engine.getSceneManager()->GetGameObjectByName("Assets/scenes/main.json", "MyModel");
        if (object) {
            object->GetComponent<TransformComponent>().addPitch(0.1f);
        }
    }
private:
};
}
