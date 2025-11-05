#include "components/SceneManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/enviroment.hpp"
#include "VirtualFileSystem.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <filesystem>

namespace vex {
    void LoadTransformComponent(GameObject& obj, const nlohmann::json& json) {
        TransformComponent comp;
        if (json.contains("position") && json["position"].is_array() && json["position"].size() >= 3) {
            comp.position = glm::vec3(
                json["position"][0].get<float>(),
                json["position"][1].get<float>(),
                json["position"][2].get<float>()
            );
        }
        if (json.contains("rotation") && json["rotation"].is_array() && json["rotation"].size() >= 3) {
            comp.rotation = glm::vec3(
                json["rotation"][0].get<float>(),
                json["rotation"][1].get<float>(),
                json["rotation"][2].get<float>()
            );
        }
        if (json.contains("scale") && json["scale"].is_array() && json["scale"].size() >= 3) {
            comp.scale = glm::vec3(
                json["scale"][0].get<float>(),
                json["scale"][1].get<float>(),
                json["scale"][2].get<float>()
            );
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
        obj.AddComponent(comp);
    }

    // Loadable Components
    REGISTER_COMPONENT(TransformComponent, LoadTransformComponent);
    REGISTER_COMPONENT(CameraComponent, LoadCameraComponent);
    REGISTER_COMPONENT(MeshComponent, LoadMeshComponent);

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

void SceneManager::AddGameObjectToScene(const std::string& scene, std::shared_ptr<GameObject> gameObject){
    m_scenes[scene]->AddGameObject(gameObject);
}

std::shared_ptr<GameObject> SceneManager::GetGameObjectByName(const std::string& scene, const std::string& name){
    return m_scenes[scene]->GetGameObjectByName(name);
}

std::shared_ptr<GameObject> SceneManager::GetGameObjectByEntity(const std::string& scene, entt::entity& entity){
    return m_scenes[scene]->GetGameObjectByEntity(entity);
}
}
