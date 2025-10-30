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

    // Loadable Objects
    REGISTER_GAME_OBJECT(CameraObject);
    REGISTER_GAME_OBJECT(GameObject);

void SceneManager::loadScene(const std::string& path, Engine& engine) {
    clearScene();
    loadSceneWithoutClearing(path, engine);
}

void SceneManager::loadSceneWithoutClearing(const std::string& path, Engine& engine) {
    //std::ifstream file(path);
    std::string realPath = GetAssetPath(path);
    if (!engine.getFileSystem()->file_exists(realPath)) {
        log("Error: Could not open scene file: %s", realPath.c_str());
        return;
    }

    auto fileData = engine.getFileSystem()->load_file(realPath);

    log("Scene data: \n%s",fileData->data.data()); // It work on my PC lol

    nlohmann::json json;
    json = nlohmann::json::parse(fileData->data.begin(), fileData->data.end(), nullptr, true);
    //json = nlohmann::json::
    //file.close();

    enviroment env;
    if (json.contains("environment") && json["environment"].contains("shading")) {
        log("Loading shading settings from scene");
        const auto& shading = json["environment"]["shading"];
        env.gourardShading = shading.value("gouraud", env.gourardShading);
        env.passiveVertexJitter = shading.value("passiveVertexJitter", env.passiveVertexJitter);
        env.vertexSnapping = shading.value("vertexSnapping", env.vertexSnapping);
        env.affineWarping = shading.value("affineTextureWarping", env.affineWarping);
        env.colorQuantization = shading.value("colorQuantization", env.colorQuantization);
        env.ntfsArtifacts = shading.value("ntfsArtifacts", env.ntfsArtifacts);
    }

    if (json.contains("environment") && json["environment"].contains("lighting")) {
        log("Loading lighting settings from scene");
        const auto& lighting = json["environment"]["lighting"];
        env.ambientLightStrength = lighting.value("ambientLightStrength", env.ambientLightStrength);

        if (lighting.contains("ambientLight") && lighting["ambientLight"].is_array() && lighting["ambientLight"].size() >= 3) {
            env.ambientLight = glm::vec3(
                lighting["ambientLight"][0].get<float>(),
                lighting["ambientLight"][1].get<float>(),
                lighting["ambientLight"][2].get<float>()
            );
        }

        if (lighting.contains("sunLight") && lighting["sunLight"].is_array() && lighting["sunLight"].size() >= 3) {
            env.sunLight = glm::vec3(
                lighting["sunLight"][0].get<float>(),
                lighting["sunLight"][1].get<float>(),
                lighting["sunLight"][2].get<float>()
            );
        }

        if (lighting.contains("sunDirection") && lighting["sunDirection"].is_array() && lighting["sunDirection"].size() >= 3) {
            env.sunDirection = glm::vec3(
                lighting["sunDirection"][0].get<float>(),
                lighting["sunDirection"][1].get<float>(),
                lighting["sunDirection"][2].get<float>()
            );
        }

        if (lighting.contains("clearColor") && lighting["clearColor"].is_array() && lighting["clearColor"].size() >= 3) {
            env.clearColor = glm::vec3(
                lighting["clearColor"][0].get<float>(),
                lighting["clearColor"][1].get<float>(),
                lighting["clearColor"][2].get<float>()
            );
        }
    }

    engine.setEnvironmentSettings(env);

    auto objects = json["objects"];
    if (!objects.is_array()) {
        log("Error: Scene file must have 'objects' array");
        return;
    }

    for (const auto& obj : objects) {
        std::string type = obj.value("type", "");
        std::string name = obj.value("name", "");
        if (type.empty() || name.empty()) {
            log("Error: Object missing type or name");
            continue;
        }

        GameObject* gameObj = GameObjectFactory::getInstance().create(type, engine, name);
        if (!gameObj) {
            log("Error: Failed to create GameObject of type '%s'", type.c_str());
            continue;
        }

        auto components = obj["components"];
        if (!components.is_array()) {
            log("Object '%s' has no components to load", name.c_str());
        } else {
            for (const auto& comp : components) {
                std::string compType = comp.value("type", "");
                if (compType.empty()) {
                    log("Error: Component missing type for object '%s'", name.c_str());
                    continue;
                }
                ComponentRegistry::getInstance().loadComponent(*gameObj, compType, comp);
            }
        }

                std::string parent = obj.value("parent", "");
                if (!parent.empty()) {
                    for(const auto& m_obj : m_objects) {
                        if (m_obj->GetComponent<NameComponent>().name == parent) {
                            gameObj->ParentTo(m_obj->GetEntity());
                            //gameObj->GetComponent<TransformComponent>().parent = m_obj->GetEntity();
                            log("Parented object '%s' to '%s'", gameObj->GetComponent<NameComponent>().name.c_str(), m_obj->GetComponent<NameComponent>().name.c_str());
                            break;
                        }
                    }
                }

        if (gameObj) {
            m_objects.emplace_back(gameObj);
        }
    }
}

void SceneManager::clearScene() {
    m_objects.clear();
}

void SceneManager::sceneBegin(){
    for (auto& obj : m_objects) {
        try{
            obj->BeginPlay();
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
    }
}

void SceneManager::sceneUpdate(float deltaTime){
    for (auto& obj : m_objects) {
        try{
            obj->Update(deltaTime);
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
    }
}

std::shared_ptr<GameObject> SceneManager::GetGameObject(const std::string& name){
    for(const auto& obj : m_objects){
        if(obj->GetComponent<NameComponent>().name == name){
            return obj;
        }
    }
    return nullptr;
}
}
