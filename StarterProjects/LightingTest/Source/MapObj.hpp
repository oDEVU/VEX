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

        vex::MeshComponent floorCubeMesh = vex::createMeshFromPath("Assets/meshes/scene.gltf", GetEngine());
        //floorCubeMesh.renderType = RenderType::TRANSPARENT;
                vex::TransformComponent cubeTransform = vex::TransformComponent{
                    GetEngine().getRegistry(),
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{0.0f, 0.0f, 0.0f},
                    glm::vec3{1.0f, 1.0f, 1.0f}};
                Model = vex::createModelFromComponents("gasStation", floorCubeMesh, cubeTransform, GetEngine());

                vex::MeshComponent frCubeMesh = vex::createMeshFromPath("Assets/meshes/scene_transparent.gltf", GetEngine());
                frCubeMesh.renderType = RenderType::TRANSPARENT;

                Transparent = vex::createModelFromComponents("gasStationTrees", frCubeMesh, cubeTransform, GetEngine());
    }

    void BeginPlay() override {

        // Yellow

        vex::LightComponent lightComp;
        lightComp.color = glm::vec3(1.0f, 0.8f, 0.15f);
        lightComp.intensity = 2.0f;
        lightComp.radius = 100.0f;

        GameObject* light1 = new GameObject(GetEngine(), "light");
        light1->AddComponent(lightComp);
        light1->AddComponent(TransformComponent(GetEngine().getRegistry(), {-34.1, 4, -18.5}));

        GameObject* light2 = new GameObject(GetEngine(), "light");
        light2->AddComponent(lightComp);
        light2->AddComponent(TransformComponent(GetEngine().getRegistry(), {-39.1, 4, -22.3}));

        GameObject* light3 = new GameObject(GetEngine(), "light");
        light3->AddComponent(lightComp);
        light3->AddComponent(TransformComponent(GetEngine().getRegistry(), {-43.9, 4, -24.1}));

                GameObject* light4 = new GameObject(GetEngine(), "light");
                light4->AddComponent(lightComp);
                light4->AddComponent(TransformComponent(GetEngine().getRegistry(), {-37.1, 4, -13.9}));

                GameObject* light5 = new GameObject(GetEngine(), "light");
                light5->AddComponent(lightComp);
                light5->AddComponent(TransformComponent(GetEngine().getRegistry(), {-42.4, 4, -17.5}));

                GameObject* light6 = new GameObject(GetEngine(), "light");
                light6->AddComponent(lightComp);
                light6->AddComponent(TransformComponent(GetEngine().getRegistry(), {-46.6, 4, -20.2}));

                GameObject* light7 = new GameObject(GetEngine(), "light");
                light7->AddComponent(lightComp);
                light7->AddComponent(TransformComponent(GetEngine().getRegistry(), {-40.0, 4, -31.5}));

                GameObject* light8 = new GameObject(GetEngine(), "light");
                light8->AddComponent(lightComp);
                light8->AddComponent(TransformComponent(GetEngine().getRegistry(), {-35.3, 4, -28.6}));

                GameObject* light9 = new GameObject(GetEngine(), "light");
                light9->AddComponent(lightComp);
                light9->AddComponent(TransformComponent(GetEngine().getRegistry(), {-30.0, 4, -25.3}));

                GameObject* light10 = new GameObject(GetEngine(), "light");
                light10->AddComponent(lightComp);
                light10->AddComponent(TransformComponent(GetEngine().getRegistry(), {-27.0, 4, -29.5}));

                                GameObject* light11 = new GameObject(GetEngine(), "light");
                                light11->AddComponent(lightComp);
                                light11->AddComponent(TransformComponent(GetEngine().getRegistry(), {-32.0, 4, -32.7}));

                                                GameObject* light12 = new GameObject(GetEngine(), "light");
                                                light12->AddComponent(lightComp);
                                                light12->AddComponent(TransformComponent(GetEngine().getRegistry(), {-36.7, 4, -36.3}));

        // Purple
        //


                lightComp.color = glm::vec3(1.0f, 0.1f, 1.0f);
                lightComp.intensity = 5.0f;
                lightComp.radius = 250.0f;

                GameObject* light13 = new GameObject(GetEngine(), "light");
                light13->AddComponent(lightComp);
                light13->AddComponent(TransformComponent(GetEngine().getRegistry(), {-44.6, 7, -11.4}));

                GameObject* light14 = new GameObject(GetEngine(), "light");
                light14->AddComponent(lightComp);
                light14->AddComponent(TransformComponent(GetEngine().getRegistry(), {-45.7, 7, -31.7}));

                GameObject* light15 = new GameObject(GetEngine(), "light");
                light15->AddComponent(lightComp);
                light15->AddComponent(TransformComponent(GetEngine().getRegistry(), {-29.4, 7, -37.8}));
    }

    void Update(float deltaTime) override {

    }
private:
vex::ModelObject* Model;
vex::ModelObject* Transparent;
};
}
