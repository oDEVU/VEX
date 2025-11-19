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
#include <vector>

namespace vex {
class PCube : public GameObject {
public:
    PCube(Engine& engine, const std::string& name) : GameObject(engine, name) {
        AddComponent(TransformComponent{GetEngine().getRegistry()});
        std::srand(std::time({}));
    }

    void BeginPlay() override {
        /*for(int i = 0; i < cubes; i++){
            ModelObject* cube = vex::createModelFromPath("Assets/meshes/cube.obj", "PCube", GetEngine());
            cube->AddComponent(vex::PhysicsComponent::Box({1.0f, 1.0f, 1.0f}, vex::BodyType::DYNAMIC, 0.25f, 0.25f));
            cube->GetComponent<TransformComponent>().addLocalPosition(glm::vec3(0.0, 2.0*i, 0.0));
            cubeObjects.push_back(cube);
        }*/
    }

    void Update(float deltaTime) override {
        if(spawned < cubes && (GetEngine().GetCurrentFrame()%5 == 0)){
            ModelObject* cube = vex::createModelFromPath("Assets/meshes/cube.obj", "PCube", GetEngine());
            cube->AddComponent(vex::PhysicsComponent::Box({1.0f, 1.0f, 1.0f}, vex::BodyType::DYNAMIC, 0.25f, 0.25f));
            cube->GetComponent<TransformComponent>().addLocalPosition(glm::vec3(0.0, 1.0*spawned, 0.0));
            cube->GetComponent<TransformComponent>().addLocalRotation(glm::vec3((std::rand()%180),(std::rand()%180),(std::rand()%180)));
            cube->GetComponent<MeshComponent>().color = glm::vec4(((float)std::rand()/(float)RAND_MAX),((float)std::rand()/(float)RAND_MAX),((float)std::rand()/(float)RAND_MAX),1.f);
            cubeObjects.push_back(cube);

            spawned++;
        }
    }
private:
    int cubes = 300, spawned = 0;
    std::vector<ModelObject*> cubeObjects;
};
}
