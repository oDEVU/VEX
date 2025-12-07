#include "components/Scene.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/enviroment.hpp"
#include "VirtualFileSystem.hpp"
#include <execution>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <exception>

namespace vex {

Scene::Scene(const std::string& path, Engine& engine) {
    m_path = path;
    m_engine = &engine;
}

Scene::~Scene() {
    m_objects.clear();
    m_addedObjects.clear();
}

void Scene::load(){
    std::string realPath = GetAssetPath(m_path);
    if (!m_engine->getFileSystem()->file_exists(realPath)) {
        log("Error: Could not open scene file: %s", realPath.c_str());
        return;
    }

    auto fileData = m_engine->getFileSystem()->load_file(realPath);

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
        env.screenQuantization = shading.value("screenQuantization", env.screenQuantization);
        env.textureQuantization = shading.value("textureQuantization", env.textureQuantization);
        env.screenDither = shading.value("screenDither", env.screenDither);
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

    m_engine->setEnvironmentSettings(env);

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

        m_creatingFromScene = true;
        GameObject* gameObj = GameObjectFactory::getInstance().create(type, *m_engine, name);
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
                                if (!m_obj || !m_obj->isValid() || !m_engine->getRegistry().valid(m_obj->GetEntity())) {
                                    continue;
                                }

                                entt::entity e = m_obj->GetEntity();
                                bool valid = m_engine->getRegistry().valid(e);

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
            //DestroyGameObject(*gameObj);
            //auto ptr = std::shared_ptr<GameObject>(gameObj);
            //m_objects.push_back(std::move(ptr));
        }
    }
}

void Scene::sceneBegin(){
    load();

    // experimental, new thing i learned idk if it will help performance at all.
    uint32_t size = m_objects.size();
    size += m_addedObjects.size();
    if(size > 50){
        std::for_each(std::execution::par_unseq, m_objects.begin(), m_objects.end(), [&](auto& obj){
            try{
                obj->BeginPlay();
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        });
        std::for_each(std::execution::par_unseq, m_addedObjects.begin(), m_addedObjects.end(), [&](auto& obj){
            try{
                obj->BeginPlay();
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        });
    }else{
        for (auto& obj : m_objects) {
            try{
                obj->BeginPlay();
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        }
        for (auto& obj : m_addedObjects) {
            try{
                obj->BeginPlay();
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        }
    }
}

void Scene::sceneUpdate(float deltaTime){
    /*for (auto& obj : m_objects) {
        try{
            obj->Update(deltaTime);
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
    }
    for (auto& obj : m_addedObjects) {
        try{
            obj->Update(deltaTime);
        }catch(const std::exception& e){
            log("Error: %s", e.what());
        }
        }*/


    uint32_t size = m_objects.size();
    size += m_addedObjects.size();
    if(size > 50){
        std::for_each(std::execution::par_unseq, m_objects.begin(), m_objects.end(), [&](auto& obj){
            try{
                obj->Update(deltaTime);
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        });
        std::for_each(std::execution::par_unseq, m_addedObjects.begin(), m_addedObjects.end(), [&](auto& obj){
            try{
                obj->Update(deltaTime);
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        });
    }else{
        for (auto& obj : m_objects) {
            try{
                obj->Update(deltaTime);
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        }
        for (auto& obj : m_addedObjects) {
            try{
                obj->Update(deltaTime);
            }catch(const std::exception& e){
                log("Error: %s", e.what());
            }
        }
    }
}

void Scene::AddGameObject(std::unique_ptr<GameObject> gameObject){
    if (gameObject) {
        m_addedObjects.push_back(std::move(gameObject));
    }
}

void Scene::RegisterGameObject(GameObject* obj) {
    if (!obj) return;
    if(m_creatingFromScene){
        m_objects.emplace_back(std::shared_ptr<GameObject>(obj));
    }else{
        m_addedObjects.emplace_back(std::shared_ptr<GameObject>(obj));
    }
    m_creatingFromScene = false;
    log("Scene adopted object: %s", obj->GetComponent<NameComponent>().name.c_str());
}

void Scene::DestroyGameObject(GameObject* obj) {
    if (!obj) return;

    for(auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        if(it->get()->GetEntity() == obj->GetEntity()) {
            m_objects.erase(it);
            break;
        }
    }

    for(auto it = m_addedObjects.begin(); it != m_addedObjects.end(); ++it) {
        if(it->get()->GetEntity() == obj->GetEntity()) {
            m_addedObjects.erase(it);
            break;
        }
    }
}

std::vector<GameObject*> Scene::GetAllGameObjectsByName(const std::string& name){
    std::vector<GameObject*> returnVector;
    for(const auto& obj : m_objects){
        if(obj->GetComponent<NameComponent>().name == name){
            returnVector.push_back(obj.get());
        }
    }
    for(const auto& obj : m_addedObjects){
        if(obj->GetComponent<NameComponent>().name == name){
            returnVector.push_back(obj.get());
        }
    }
    return returnVector;
}

std::vector<GameObject*> Scene::GetAllGameObjectsByClassName(const std::string& classname){
    std::vector<GameObject*> returnVector;
    for(const auto& obj : m_objects){
        if(obj->getObjectType() == classname){
            returnVector.push_back(obj.get());
        }
    }
    for(const auto& obj : m_addedObjects){
        if(obj->getObjectType() == classname){
            returnVector.push_back(obj.get());
        }
    }
    return returnVector;
}

GameObject* Scene::GetGameObjectByEntity(entt::entity& entity){
    for(const auto& obj : m_objects){
        if(obj->GetEntity() == entity){
            return obj.get();
        }
    }
    for(const auto& obj : m_addedObjects){
        if(obj->GetEntity() == entity){
            return obj.get();
        }
    }
    return nullptr;
}

void Scene::Save(const std::string& path) {
    nlohmann::json sceneJson;

    const auto& env = m_engine->getEnvironmentSettings();
    sceneJson["environment"]["lighting"] = {
        {"ambientLightStrength", env.ambientLightStrength},
        {"ambientLight", {env.ambientLight.r, env.ambientLight.g, env.ambientLight.b}},
        {"sunLight", {env.sunLight.r, env.sunLight.g, env.sunLight.b}},
        {"sunDirection", {env.sunDirection.x, env.sunDirection.y, env.sunDirection.z}},
        {"clearColor", {env.clearColor.r, env.clearColor.g, env.clearColor.b}}
    };

    sceneJson["environment"]["shading"] = {
        {"gouraud", env.gourardShading},
        {"passiveVertexJitter", env.passiveVertexJitter},
        {"vertexSnapping", env.vertexSnapping},
        {"affineTextureWarping", env.affineWarping},
        {"screenQuantization", env.screenQuantization},
        {"textureQuantization", env.textureQuantization},
        {"screenDither", env.screenDither},
        {"ntfsArtifacts", env.ntfsArtifacts}
    };

    std::unordered_map<entt::entity, std::vector<GameObject*>> hierarchyMap;
    std::vector<GameObject*> rootObjects;

    for (const auto& objPtr : m_objects) {
        if (!objPtr || !objPtr->isValid()) continue;

        GameObject* obj = objPtr.get();
        entt::entity parentEntity = entt::null;

        if (obj->HasComponent<TransformComponent>()) {
            parentEntity = obj->GetComponent<TransformComponent>().getParent();
        }

        if (parentEntity != entt::null && m_engine->getRegistry().valid(parentEntity)) {
            hierarchyMap[parentEntity].push_back(obj);
        } else {
            rootObjects.push_back(obj);
        }
    }

    std::vector<GameObject*> sortedObjects;
    std::deque<GameObject*> processQueue;

    for (auto* root : rootObjects) {
        processQueue.push_back(root);
    }

    while (!processQueue.empty()) {
        GameObject* currentObj = processQueue.front();
        processQueue.pop_front();

        sortedObjects.push_back(currentObj);

        entt::entity currentEntity = currentObj->GetEntity();

        if (hierarchyMap.find(currentEntity) != hierarchyMap.end()) {
            for (auto* child : hierarchyMap[currentEntity]) {
                processQueue.push_back(child);
            }
        }
    }

    nlohmann::json objectsArray = nlohmann::json::array();

    for (GameObject* obj : sortedObjects) {
        nlohmann::json objJson;

        std::string name = obj->GetComponent<NameComponent>().name;
        objJson["name"] = name;
        objJson["type"] = obj->getObjectType();

        if (obj->HasComponent<TransformComponent>()) {
            entt::entity parentEntity = obj->GetComponent<TransformComponent>().getParent();
            if (parentEntity != entt::null && m_engine->getRegistry().valid(parentEntity)) {
                GameObject* parentObj = GetGameObjectByEntity(parentEntity);
                if (parentObj) {
                    objJson["parent"] = parentObj->GetComponent<NameComponent>().name;
                }
            }
        }

        nlohmann::json componentsArray = nlohmann::json::array();
        const auto& regNames = ComponentRegistry::getInstance().getRegisteredNames();

        for (const auto& compName : regNames) {
            nlohmann::json compData = ComponentRegistry::getInstance().saveComponent(*obj, compName);
            if (!compData.is_null()) {
                compData["type"] = compName;
                componentsArray.push_back(compData);
            }
        }

        objJson["components"] = componentsArray;
        objectsArray.push_back(objJson);
    }

    sceneJson["objects"] = objectsArray;

    std::ofstream file(path);
    if (file.is_open()) {
        file << std::setw(4) << sceneJson << std::endl;
        log("Scene saved successfully to: %s", path.c_str());
    } else {
        log("Error: Failed to write scene file to: %s", path.c_str());
    }
}

void Scene::AddEditorGameObject(GameObject* gameObject) {
    if (!gameObject) return;

    auto it = std::find_if(m_addedObjects.begin(), m_addedObjects.end(),
        [gameObject](const std::shared_ptr<GameObject>& ptr) {
            return ptr.get() == gameObject;
        });

    if (it != m_addedObjects.end()) {
        m_objects.push_back(*it);
        m_addedObjects.erase(it);

        log("Promoted object to persistent Scene: %s", gameObject->GetComponent<NameComponent>().name.c_str());
    } else {
        m_objects.emplace_back(std::shared_ptr<GameObject>(gameObject));
        log("Added new persistent object to Scene: %s", gameObject->GetComponent<NameComponent>().name.c_str());
    }
}
}
