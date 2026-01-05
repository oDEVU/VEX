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
        AddComponent(TransformComponent{GetEngine().getRegistry()});

        vex::MeshComponent floorCubeMesh = vex::createMeshFromPath("Assets/meshes/cube.obj", GetEngine());
                vex::TransformComponent cubeTransform = vex::TransformComponent{
                    GetEngine().getRegistry(),
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{10.0f, 0.5f, 10.0f}};
                Model = vex::createModelFromComponents("cube", floorCubeMesh, cubeTransform, GetEngine());
                vex::PhysicsComponent cubePhysics = vex::PhysicsComponent::Box({10.0f, 0.5f, 10.0f}, vex::BodyType::STATIC);

                AddComponent(cubePhysics);

                vex::TransformComponent wall12Transform = vex::TransformComponent{
                    GetEngine().getRegistry(),
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.5f, 10.0f, 10.0f}};
                wall12Transform.addLocalPosition(glm::vec3(10.f,0.f,0.f));
                wall1 = vex::createModelFromComponents("cube", floorCubeMesh, wall12Transform, GetEngine());
                wall12Transform.addLocalPosition(glm::vec3(-20.f,0.f,0.f));
                wall2 = vex::createModelFromComponents("cube", floorCubeMesh, wall12Transform, GetEngine());
                vex::PhysicsComponent wal12Physics = vex::PhysicsComponent::Box({0.5f, 10.0f, 10.0f}, vex::BodyType::STATIC);

                wall1->AddComponent(wal12Physics);
                wall2->AddComponent(wal12Physics);

                vex::TransformComponent wall34Transform = vex::TransformComponent{
                    GetEngine().getRegistry(),
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{10.0f, 10.0f, 0.5f}};
                wall34Transform.addLocalPosition(glm::vec3(0.f,0.f,10.f));
                wall3 = vex::createModelFromComponents("cube", floorCubeMesh, wall34Transform, GetEngine());
                wall34Transform.addLocalPosition(glm::vec3(0.f,0.f,-20.f));
                wall4 = vex::createModelFromComponents("cube", floorCubeMesh, wall34Transform, GetEngine());
                vex::PhysicsComponent wal34Physics = vex::PhysicsComponent::Box({10.0f, 10.0f, 0.5f}, vex::BodyType::STATIC);

                wall3->AddComponent(wal34Physics);
                wall4->AddComponent(wal34Physics);
    }

    void BeginPlay() override {
    }

    void Update(float deltaTime) override {

    }

    ~Map() {
        // Theres no need to delete these objects cause scene already takes care of it. Additionally it can cause double free if you do it.
    }
private:
vex::ModelObject* Model;
vex::ModelObject* wall1;
vex::ModelObject* wall2;
vex::ModelObject* wall3;
vex::ModelObject* wall4;
};
}
