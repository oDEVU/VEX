#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/UiComponent.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"

#include "components/PhysicsSystem.hpp"
#include <cstdlib>
#include <memory>
#include <string>

namespace vex {
class Map : public GameObject {
public:
    Map(Engine& engine, const std::string& name) : GameObject(engine, name) {

    }

    void BeginPlay() override {
        vex::MeshComponent humanMesh = vex::createMeshFromPath("Assets/human.obj", GetEngine());
        vex::TransformComponent humanTransform = vex::TransformComponent{
            GetEngine().getRegistry(),
            glm::vec3{0.0f, 0.3f, 0.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{1.0f, 1.0f, 1.0f}};
        //vex::PhysicsComponent humanPhysics = vex::PhysicsComponent::Mesh(humanMesh, vex::BodyType::KINEMATIC);
        vex::PhysicsComponent humanPhysics = vex::PhysicsComponent::Box({0.5f, 1.0f, 0.1f}, vex::BodyType::KINEMATIC);

        humanEntity1 = vex::createModelFromComponents("human1", humanMesh, humanTransform, GetEngine());
        humanTransform.addLocalPosition(glm::vec3(1.0f,-0.3f,0));
        humanEntity2 = vex::createModelFromComponents("human2", humanMesh, humanTransform, GetEngine());
        //humanTransform.addLocalPosition(glm::vec3(0.5f,0,0));
        humanEntity3 = vex::createModelFromComponents("human3", humanMesh, humanTransform, GetEngine());
        //humanTransform.addLocalPosition(glm::vec3(0.5f,0,0));
        humanEntity4 = vex::createModelFromComponents("human4", humanMesh, humanTransform, GetEngine());

        humanEntity2->ParentTo(humanEntity1->GetEntity());
        humanEntity3->ParentTo(humanEntity2->GetEntity());
        humanEntity4->ParentTo(humanEntity3->GetEntity());

        humanEntity1->AddComponent(humanPhysics);
        humanEntity2->AddComponent(humanPhysics);
        humanEntity3->AddComponent(humanPhysics);
        humanEntity4->AddComponent(humanPhysics);

        viperEntity1 = vex::createModelFromPath("Assets/scene.gltf", "viper1", GetEngine());
        viperEntity2 = vex::createModelFromPath("Assets/scene.gltf", "viper2", GetEngine());
        viperEntity2->GetComponent<vex::TransformComponent>().addLocalPosition(glm::vec3{2.0f, 0.0f, 0.0f});

        vex::PhysicsComponent viperPhysics = vex::PhysicsComponent::Mesh(viperEntity1->GetComponent<vex::MeshComponent>(), vex::BodyType::STATIC);

        viperEntity1->AddComponent(viperPhysics);
        viperEntity2->AddComponent(viperPhysics);


        viperShadow1 = vex::createModelFromPath("Assets/ViperShadow.obj", "shadow1", GetEngine());
        viperShadow1->GetComponent<vex::MeshComponent>().renderType = vex::RenderType::TRANSPARENT;
        viperShadow1->GetComponent<vex::TransformComponent>().addLocalRotation(glm::vec3{0.0f, 90.0f, 0.0f});
        viperShadow1->GetComponent<vex::TransformComponent>().setLocalScale(glm::vec3{0.9f, 0.9f, 0.9f});
        viperShadow1->GetComponent<vex::TransformComponent>().addLocalPosition(glm::vec3{0.0f, -0.03f, 0.0f});
        viperShadow2 = vex::createModelFromPath("Assets/ViperShadow.obj", "shadow2", GetEngine());
        viperShadow2->GetComponent<vex::MeshComponent>().renderType = vex::RenderType::TRANSPARENT;
        viperShadow2->GetComponent<vex::TransformComponent>().addLocalRotation(glm::vec3{0.0f, 90.0f, 0.0f});
        viperShadow2->GetComponent<vex::TransformComponent>().setLocalScale(glm::vec3{0.9f, 0.9f, 0.9f});
        viperShadow2->GetComponent<vex::TransformComponent>().addLocalPosition(glm::vec3{2.0f, -0.03f, 0.0f});



        vex::MeshComponent floorCubeMesh = vex::createMeshFromPath("Assets/floorCube.obj", GetEngine());
        vex::TransformComponent cubeTransform = vex::TransformComponent{
            GetEngine().getRegistry(),
            glm::vec3{0.5f, -1.1f, 0.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{5.0f, 0.5f, 5.0f}};
        cube = vex::createModelFromComponents("cube", floorCubeMesh, cubeTransform, GetEngine());
        vex::PhysicsComponent cubePhysics = vex::PhysicsComponent::Box({5.0f, 0.5f, 5.0f}, vex::BodyType::STATIC);

        cubePhysics.addCollisionEnterBinding([this](entt::entity self, entt::entity other, const vex::CollisionHit& hit) {
            log("Hit at (%f, %f, %f)", hit.position.x, hit.position.y, hit.position.z);
        });
        cubePhysics.addCollisionStayBinding([this](entt::entity self, entt::entity other, const vex::CollisionHit& hit) {
            log("Still touching");
        });
        cubePhysics.addCollisionExitBinding([this](entt::entity self, entt::entity other) {
            log("Stopped touching");
        });

        cube->AddComponent(cubePhysics);

        vex::MeshComponent cubeMesh = vex::createMeshFromPath("Assets/cube.obj", GetEngine());

        vex::TransformComponent cube2Transform = vex::TransformComponent{
            GetEngine().getRegistry(),
            glm::vec3{1.0f, 10.0f, 1.0f},
            glm::vec3{0.0f, 0.0f, 0.0f},
            glm::vec3{0.5f, 0.5f, 0.5f}};
        cube2 = vex::createModelFromComponents("cube2", cubeMesh, cube2Transform, GetEngine());
        vex::PhysicsComponent cube2Physics = vex::PhysicsComponent::Box({0.5f, 0.5f, 0.5f}, vex::BodyType::DYNAMIC);
        cube2->AddComponent(cube2Physics);

        cube2Transform.addLocalPosition(glm::vec3{0.25f, 3.0f, 0.0f});
        cube2Transform.setLocalRotation(glm::vec3{-30.0f, 45.0f, 5.0f});
        cube3 = vex::createModelFromComponents("cube3", cubeMesh, cube2Transform, GetEngine());
        cube3->AddComponent(cube2Physics);

        cube2Transform.addLocalPosition(glm::vec3{-0.5f, 3.0f, 0.2f});
        cube2Transform.setLocalRotation(glm::vec3{15.0f, 15.0f, 30.0f});
        cube4 = vex::createModelFromComponents("cube4", cubeMesh, cube2Transform, GetEngine());
        cube4->AddComponent(cube2Physics);

        vex::InputComponent inputComp;
        inputComp.addBinding(SDL_SCANCODE_SPACE, vex::InputActionState::Held, [this](float deltaTime) {
            //GetComponent<vex::TransformComponent>().position += GetComponent<vex::TransformComponent>().getForwardVector(m_engine.GetEngine().getRegistry()) * (deltaTime * 2);
            GetEngine().getPhysicsSystem()->AddImpulse(cube4->GetComponent<vex::PhysicsComponent>().bodyId, glm::vec3(0.0f, 0.1f, 0.0f));
        });
        cube4->AddComponent(inputComp);

        testLight->AddComponent(vex::TransformComponent{GetEngine().getRegistry()});
        testLight->GetComponent<vex::TransformComponent>().addLocalPosition({1, 1.5f, 0});

        vex::LightComponent lightComp;
        lightComp.color = glm::vec3(1.0f, 0.0f, 1.0f);
        lightComp.intensity = 1.0f;
        lightComp.radius = 15.0f;
        testLight->AddComponent(lightComp);
//Fix imgui for new system
/*#if DEBUG
        m_imgui->addUIFunction([this]() {
            ImGui::Begin("Engine Stats");
            ImGui::Text("Renderer: Vulkan");
            ImGui::Text("FPS: %i", (int)fps);
            ImGui::Text("Current frame: %i", (int)m_frame);
            ImGui::Text("Render resolution: %ix%i", (int)this->m_resolutionManager->getRenderResolution().x, (int)this->m_resolutionManager->getRenderResolution().y);
            ImGui::Text("Window resolution: %ix%i", (int)this->m_resolutionManager->getWindowResolution().x, (int)this->m_resolutionManager->getWindowResolution().y);
            ImGui::End();
        });
#endif*/
    }

    void Update(float deltaTime) override {
        if (humanEntity1 && humanEntity1->isValid()) {
            humanEntity1->GetComponent<vex::TransformComponent>().addYaw(25*deltaTime);
        }
    }

    ~Map() {
        delete humanEntity1;
        delete humanEntity2;
        delete humanEntity3;
        delete humanEntity4;
        delete viperEntity1;
        delete viperEntity2;
        delete viperShadow1;
        delete viperShadow2;
        delete cube;
        delete cube2;
        delete cube3;
        delete cube4;
    }
private:
vex::ModelObject* humanEntity1;
vex::ModelObject* humanEntity2;
vex::ModelObject* humanEntity3;
vex::ModelObject* humanEntity4;
vex::ModelObject* viperEntity1;
vex::ModelObject* viperEntity2;
vex::ModelObject* viperShadow1;
vex::ModelObject* viperShadow2;
vex::ModelObject* cube;
vex::ModelObject* cube2;
vex::ModelObject* cube3;
vex::ModelObject* cube4;
vex::GameObject* testLight = new vex::GameObject(GetEngine(), "TestLight");

};
}
