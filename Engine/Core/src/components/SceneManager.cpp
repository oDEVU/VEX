#include "components/SceneManager.hpp"
#include "VirtualFileSystem.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <filesystem>

namespace vex {

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

    SDL_Log("%s",fileData->data.data());

    nlohmann::json json;
    json = nlohmann::json::parse(fileData->data.data(), nullptr, true);
    //json = nlohmann::json::
    //file.close();

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
        obj->BeginPlay();
    }
}

void SceneManager::sceneUpdate(float deltaTime){
    for (auto& obj : m_objects) {
        obj->Update(deltaTime);
    }
}

}
