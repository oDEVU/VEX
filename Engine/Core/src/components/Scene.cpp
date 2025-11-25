#include "components/Scene.hpp"
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

Scene::Scene(const std::string& path, Engine& engine) {
    //std::ifstream file(path);
    std::string realPath = GetAssetPath(path);
    if (!engine.getFileSystem()->file_exists(realPath)) {
        log("Error: Could not open scene file: %s", realPath.c_str());
        return;
    }

    auto fileData = engine.getFileSystem()->load_file(realPath);

    log("Scene data: \n%s",fileData->data.data()); // It works on my PC lol, (this comment exists cause there was a compter that had seqfault here couple times)

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
                            bool parentFound = false;
                            for(const auto& m_obj : m_objects) {
                                // FIX: Check if m_obj is valid and its entity exists in registry
                                if (!m_obj || !m_obj->isValid() || !engine.getRegistry().valid(m_obj->GetEntity())) {
                                    continue;
                                }

                                entt::entity e = m_obj->GetEntity();
                                bool valid = engine.getRegistry().valid(e);

                                log("Checking Object at %p | Entity ID: %u (Hex: %X) | Valid in Registry: %d",
                                        (void*)m_obj.get(), (uint32_t)e, (uint32_t)e, valid);

                                // Safe to access component now
                                if (m_obj->GetComponent<NameComponent>().name == parent) {
                                    gameObj->ParentTo(m_obj->GetEntity());
                                    log("Parented object '%s' to '%s'", name.c_str(), parent.c_str());
                                    parentFound = true;
                                    break;
                                }
                            }
                            if (!parentFound) {
                                log("Warning: Parent '%s' not found for object '%s'", parent.c_str(), name.c_str());
                            }
                        }

        if (gameObj) {
            auto ptr = std::unique_ptr<GameObject>(gameObj);
            m_objects.push_back(std::move(ptr));
        }
    }
}

Scene::~Scene() {
    m_objects.clear();
}

void Scene::sceneBegin(){
    for (auto& obj : m_objects) {
        try{
            obj->BeginPlay();
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
    }
}

void Scene::sceneUpdate(float deltaTime){
    for (auto& obj : m_objects) {
        try{
            obj->Update(deltaTime);
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
    }
}

void Scene::AddGameObject(std::unique_ptr<GameObject> gameObject){
    if (gameObject) {
        m_objects.push_back(std::move(gameObject));
    }
}

GameObject* Scene::GetGameObjectByName(const std::string& name){
    for(const auto& obj : m_objects){
        if(obj->GetComponent<NameComponent>().name == name){
            return obj.get();
        }
    }
    return nullptr;
}

GameObject* Scene::GetGameObjectByEntity(entt::entity& entity){
    for(const auto& obj : m_objects){
        if(obj->GetEntity() == entity){
            return obj.get();
        }
    }
    return nullptr;
}
}
