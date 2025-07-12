#include "MeshManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <fstream>
#include <unordered_set>
#include <SDL3/SDL.h>

namespace vex {
    MeshManager::MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources)
        : m_r_context(context), m_p_resources(resources) {
        log("MeshManager initialized");
    }

    MeshManager::~MeshManager() {
        m_vulkanMeshes.clear();
        log("MeshManager destroyed");
    }

    ModelObject* MeshManager::createModel(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent = entt::null){
        log("Constructing model: %s...", name.c_str());

        auto view = engine.getRegistry().view<NameComponent>();
        for (auto entity : view) {
            auto meshComponent = view.get<NameComponent>(entity);
            if(meshComponent.name == name){
                throw_error("Object with name: '" + name + "' already exists");
            }
        }

        uint32_t newId;
        if (!m_freeModelIds.empty()) {
            newId = m_freeModelIds.back();
            m_freeModelIds.pop_back();
        } else {
            newId = m_nextModelId++;
            if (newId >= m_r_context.MAX_MODELS) {
                throw_error("Maximum model count exceeded");
            }
        }

        meshComponent.id = newId;

                std::unordered_set<std::string> uniqueTextures;
                for (const auto& submesh : meshComponent.meshData.submeshes) {
                    if (!submesh.texturePath.empty()) {
                        uniqueTextures.insert(submesh.texturePath);
                    }
                }

                log("Loading %zu submesh textures", uniqueTextures.size());
                for (const auto& texPath : uniqueTextures) {
                    log("Processing texture: %s", texPath.c_str());
                    if (!m_p_resources->textureExists(texPath)) {
                        try {
                            m_p_resources->loadTexture(texPath, texPath);
                            log("Loaded texture: %s", texPath.c_str());
                        } catch (const std::exception& e) {
                            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                                         "Failed to load texture %s",
                                         texPath.c_str());
                            handle_exception(e);
                        }
                    } else {
                        log("Texture already exists: %s", texPath.c_str());
                    }
                }

        try {
            log("Creating Vulkan mesh for %s", name.c_str());
            m_vulkanMeshes.push_back(std::make_unique<VulkanMesh>(m_r_context));
            m_vulkanMeshes.back()->upload(meshComponent.meshData);
            log("Mesh upload successful");
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Mesh upload failed");
            m_vulkanMeshes.pop_back();
            handle_exception(e);
        }

        entt::entity modelEntity = engine.getRegistry().create();
        ModelObject* modelObject = new ModelObject(engine, name, meshComponent, transformComponent);

        return modelObject;
    }

    MeshComponent MeshManager::loadMesh(const std::string& path) {
        /*
        log("Loading model: %s...", name.c_str());

        auto view = engine.getRegistry().view<MeshComponent>();
        for (auto entity : view) {
            auto meshComponent = view.get<MeshComponent>(entity);
            if(meshComponent.modelName == name){
                throw_error("Model '" + name + "' already exists");
            }
        }
        //if (m_modelRegistry.count(name)) {
        //    throw_error("Model '" + name + "' already exists");
        //}
        */
        MeshData meshData;
        MeshComponent meshComponent;
        try {

            log("Loading mesh data from: %s", path.c_str());
            std::ifstream fileCheck(path);
            if (!fileCheck.is_open()) {
                throw_error("File not found: " + path);
            }
            fileCheck.close();
            meshData.loadFromFile(path);
        } catch (const std::exception& e) {
            SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Mesh load failed");
            handle_exception(e);
        }

        /*
        uint32_t newId;
        if (!m_freeModelIds.empty()) {
            newId = m_freeModelIds.back();
            m_freeModelIds.pop_back();
        } else {
            newId = m_nextModelId++;
            if (newId >= m_r_context.MAX_MODELS) {
                throw_error("Maximum model count exceeded");
            }
        }
*/
        //meshComponent.id = newId;
        meshComponent.meshData = std::move(meshData);

        /*
        try {
            log("Creating Vulkan mesh for %s", path.c_str());
            m_vulkanMeshes.push_back(std::make_unique<VulkanMesh>(m_r_context));
            m_vulkanMeshes.back()->upload(meshComponent.meshData);
            log("Mesh upload successful");
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Mesh upload failed");
            m_vulkanMeshes.pop_back();
            handle_exception(e);
        }
        */

        //TransformComponent transformComponent;
        //entt::entity modelEntity = engine.getRegistry().create();

        //ModelObject* modelObject = new ModelObject(engine, Mesh, transformComponent);

        //log("Model %s registered successfully", name.c_str());
        return meshComponent;//engine.getRegistry().get<GameObjectComponent>(modelObject->GetEntity()).object.get();
    }

    void MeshManager::destroyModel(ModelObject& modelObject) {
        // TODO

        /*auto it = m_modelRegistry.find(name);
        if (it == m_modelRegistry.end()) return;

        m_freeModelIds.push_back(it->second->id);

        auto modelIter = std::find_if(
            m_models.begin(), m_models.end(),
            [&](const auto& m) { return m.get() == it->second; }
        );
        if (modelIter != m_models.end()) {
            m_vulkanMeshes.erase(m_vulkanMeshes.begin() + (modelIter - m_models.begin()));
            m_models.erase(modelIter);
        }

        m_modelRegistry.erase(name);
        m_p_resources->unloadTexture(name);*/
        //modelObject.destroy();
    }
}
