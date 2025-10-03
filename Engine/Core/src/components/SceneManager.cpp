#include "components/SceneManager.hpp"
#include "components/enviroment.hpp"
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

    log("Scene data: \n%s",fileData->data.data()); // It work on my PC lol

    nlohmann::json json;
    json = nlohmann::json::parse(fileData->data.data(), nullptr, true);
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
