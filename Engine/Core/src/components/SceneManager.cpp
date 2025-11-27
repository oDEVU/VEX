#include "components/SceneManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/PhysicsSystem.hpp"
#include "components/enviroment.hpp"
#include "VirtualFileSystem.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <filesystem>

namespace vex {
    void LoadTransformComponent(GameObject& obj, const nlohmann::json& json) {
        TransformComponent comp(obj.GetEngine().getRegistry());
        if (json.contains("position") && json["position"].is_array() && json["position"].size() >= 3) {
            glm::vec3 position(
                json["position"][0].get<float>(),
                json["position"][1].get<float>(),
                json["position"][2].get<float>()
            );
            comp.setLocalPosition(position);
        }
        if (json.contains("rotation") && json["rotation"].is_array() && json["rotation"].size() >= 3) {
            glm::vec3 rotation(
                json["rotation"][0].get<float>(),
                json["rotation"][1].get<float>(),
                json["rotation"][2].get<float>()
            );
            comp.setLocalRotation(rotation);
        }
        if (json.contains("scale") && json["scale"].is_array() && json["scale"].size() >= 3) {
            glm::vec3 scale(
                json["scale"][0].get<float>(),
                json["scale"][1].get<float>(),
                json["scale"][2].get<float>()
            );
            comp.setLocalScale(scale);
        }
        obj.AddComponent(comp);
    }

    void LoadCameraComponent(GameObject& obj, const nlohmann::json& json) {
        CameraComponent comp;
        if (json.contains("fov") && !json["fov"].is_array()) {
            comp.fov = json["fov"].get<float>();
        }
        if (json.contains("nearPlane") && !json["nearPlane"].is_array()) {
            comp.nearPlane = json["nearPlane"].get<float>();
        }
        if (json.contains("farPlane") && !json["farPlane"].is_array()) {
            comp.farPlane = json["farPlane"].get<float>();
        }
        obj.AddComponent(comp);
    }

    void LoadMeshComponent(GameObject& obj, const nlohmann::json& json) {
        MeshComponent comp;
        if (json.contains("path") && !json["path"].is_array()) {
            comp = createMeshFromPath(json["path"], obj.GetEngine());
        }
        if (json.contains("transparent") && !json["transparent"].is_array()) {
            bool transparent = json["transparent"].get<bool>();
            if(transparent){
                comp.renderType = RenderType::TRANSPARENT;
            }
        }
        obj.AddComponent(comp);
    }

    void LoadPhysicsComponent(GameObject& obj, const nlohmann::json& json) {
        // TODO: parsing logic later, added to test editor.
        vex::PhysicsComponent pc;
        obj.AddComponent(pc);
    }

    REGISTER_COMPONENT(TransformComponent, LoadTransformComponent);
    REGISTER_COMPONENT(CameraComponent, LoadCameraComponent);
    REGISTER_COMPONENT(MeshComponent, LoadMeshComponent);

    #if DEBUG
    template<>
    void vex::GenericComponentInspector<vex::PhysicsComponent>(GameObject& obj) {
        if (obj.HasComponent<vex::PhysicsComponent>()) {
            if (ImGui::CollapsingHeader("PhysicsComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& pc = obj.GetComponent<vex::PhysicsComponent>();

                ImReflect::Input("Shape", pc.shape); // Draw Enum

                if (pc.shape == ShapeType::BOX)
                    ImGui::DragFloat3("Extents", &pc.boxHalfExtents.x);
                else if (pc.shape == ShapeType::SPHERE)
                    ImGui::DragFloat("Radius", &pc.sphereRadius);
                else if (pc.shape == ShapeType::CAPSULE)
                    ImGui::DragFloat2("Capsule", &pc.capsuleRadius);
                else if (pc.shape == ShapeType::CYLINDER)
                    ImGui::DragFloat2("Cylinder", &pc.cylinderRadius);

                ImReflect::Input("Mass", pc.mass);
                ImReflect::Input("Friction", pc.friction);
                ImReflect::Input("Bounciness", pc.bounce);
                ImReflect::Input("Linear Damping", pc.linearDamping);
                ImReflect::Input("Angular Damping", pc.angularDamping);
                ImReflect::Input("Sensor", pc.isSensor);
                ImReflect::Input("Allow Sleeping", pc.allowSleeping);

                if (ImGui::Button("Remove")) {
                    obj.GetEngine().getRegistry().remove<vex::PhysicsComponent>(obj.GetEntity());
                }
            }
        }
    }
    #endif

    REGISTER_COMPONENT(PhysicsComponent, LoadPhysicsComponent);

    /// @todo Add physics component loading function

    // Loadable Objects
    REGISTER_GAME_OBJECT(CameraObject);
    REGISTER_GAME_OBJECT(GameObject);

void SceneManager::loadScene(const std::string& path, Engine& engine) {
    clearScenes();
    loadSceneWithoutClearing(path, engine);
}

void SceneManager::unloadScene(const std::string& path) {
    m_scenes.erase(path);
}

void SceneManager::loadSceneWithoutClearing(const std::string& path, Engine& engine) {
    m_scenes.emplace(path, std::make_shared<Scene>(path, engine));
    #if DEBUG
    lastSceneName = path;
    #endif
    m_scenes[path]->sceneBegin();
}

void SceneManager::clearScenes() {
    m_scenes.clear();
}

void SceneManager::scenesUpdate(float deltaTime){
    for (auto& scene : m_scenes) {
        scene.second->sceneUpdate(deltaTime);
    }
}

void SceneManager::AddGameObjectToScene(const std::string& scene, std::unique_ptr<GameObject> gameObject){
    m_scenes[scene]->AddGameObject(std::move(gameObject));
}

GameObject* SceneManager::GetGameObjectByName(const std::string& scene, const std::string& name){
    return m_scenes[scene]->GetGameObjectByName(name);
}

GameObject* SceneManager::GetGameObjectByEntity(const std::string& scene, entt::entity& entity){
    return m_scenes[scene]->GetGameObjectByEntity(entity);
}

std::vector<std::string> SceneManager::GetAllSceneNames() const {
    std::vector<std::string> names;
    names.reserve(m_scenes.size());

    for (const auto& [name, scene] : m_scenes) {
        names.push_back(name);
    }

    return names;
}
}
