#include "MeshManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <fstream>
#include <iterator>
#include <unordered_set>
#include <SDL3/SDL.h>
#include "limits.hpp"

namespace vex {
    MeshManager::MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources, VirtualFileSystem* vfs)
        : m_r_context(context), m_p_resources(resources), m_vfs(vfs) {
        log("MeshManager initialized");
    }

    MeshManager::~MeshManager() {
        m_vulkanMeshes.clear();
        log("MeshManager destroyed");
    }

    ModelObject* MeshManager::createModel(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, entt::entity parent = entt::null){
        log("Constructing model: %s...", name.c_str());

        std::string tempName = name;

        auto view = m_p_engine->getRegistry().view<NameComponent>();
        for (auto entity : view) {
            auto meshComponent = view.get<NameComponent>(entity);
            if(meshComponent.name == tempName){
                log("Warning: Object with name: '%s already exists! All objects should have unique names.", tempName.c_str());
                tp::UUID uuidGenerator;
                tempName = tempName + uuidGenerator.generate_uuid();
                log("Info: Object created with name: '%s', it is still recommended to not rely on this name for identification. It is different every app run.", tempName.c_str());
            }
        }
        //m_m_p_engine->getRegistry().emplace<NameComponent>(m_entity, tempName);

        uint32_t newId;
        if (!m_freeModelIds.empty()) {
            newId = m_freeModelIds.back();
            m_freeModelIds.pop_back();
        } else {
            newId = m_nextModelId++;
            if (newId >= MAX_MODELS) {
                throw_error("Maximum model count exceeded");
            }
        }
        //std::string realPath = GetAssetPath(path);
        meshComponent.id = newId;
        meshComponent.textureNames.clear();

        //std::filesystem::path meshPath(meshComponent.meshData.meshPath);
        //meshPath.remove_filename();

                std::unordered_set<std::string> uniqueTextures;
                for (const auto& submesh : meshComponent.meshData.submeshes) {
                    if (!submesh.texturePath.empty()) {
                        uniqueTextures.insert(submesh.texturePath);
                        meshComponent.textureNames.push_back(submesh.texturePath);
                    }
                }

                log("Loading %zu submesh textures", uniqueTextures.size());
                for (const auto& texPath : uniqueTextures) {
                    log("Processing texture: %s", texPath.c_str());
                    if (!m_p_resources->textureExists(texPath)) {
                        try {
                            m_p_resources->loadTexture(texPath, texPath, m_vfs);
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
            log("Creating Vulkan mesh for %s", tempName.c_str());
            if(m_vulkanMeshes.find(meshComponent.meshData.meshPath) == m_vulkanMeshes.end()){
                m_vulkanMeshes.emplace(meshComponent.meshData.meshPath, std::make_unique<VulkanMesh>(m_r_context));
                m_vulkanMeshes.at(meshComponent.meshData.meshPath)->upload(meshComponent.meshData);
                m_vulkanMeshes.at(meshComponent.meshData.meshPath)->addInstance();
            }else{
                m_vulkanMeshes.at(meshComponent.meshData.meshPath)->addInstance();
                log("Reusing same mesh model");
            }
            //m_vulkanMeshes.push_back(std::make_unique<VulkanMesh>(m_r_context));
            //log("vulkanMesh id: %i", m_vulkanMeshes.size());
            log("Mesh upload successful");
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Mesh upload failed");
            m_vulkanMeshes.erase(meshComponent.meshData.meshPath);
            handle_exception(e);
        }

        //entt::entity modelEntity = m_p_engine->getRegistry().create();
        ModelObject* modelObject = new ModelObject(*m_p_engine, tempName, meshComponent, transformComponent);
        modelObject->cleanup = [this](std::string& tempName, MeshComponent meshComponent) { destroyModel(tempName, meshComponent); };
        return modelObject;
    }

    MeshComponent MeshManager::loadMesh(const std::string& path) {
        MeshData meshData;
        MeshComponent meshComponent;
        std::string realPath = GetAssetPath(path);

        try {
            log("Loading mesh data from: %s", realPath.c_str());

            if (!m_vfs->file_exists(realPath)) {
                throw_error("File not found: " + realPath);
            }

            meshData.loadFromFile(realPath, m_vfs);
            meshData.meshPath = path;
        } catch (const std::exception& e) {
            SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Mesh load failed");
            handle_exception(e);
        }

        meshComponent.meshData = std::move(meshData);

        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);

        for(auto& submesh : meshComponent.meshData.submeshes) {
            for(auto& vertex : submesh.vertices) {
                min = glm::min(min, vertex.position);
                max = glm::max(max, vertex.position);
            }
        }

        glm::vec3 center = (min + max) * 0.5f;
        float radius = glm::length(max - center);

        meshComponent.localCenter = center;
        meshComponent.localRadius = radius;

        return meshComponent;
    }

    std::unique_ptr<VulkanMesh>& MeshManager::getVulkanMeshByMesh(MeshComponent& meshComponent) {
        if (!m_vulkanMeshes.count(meshComponent.meshData.meshPath)){
            meshComponent = loadMesh(meshComponent.meshData.meshPath);
        }
        return m_vulkanMeshes.at(meshComponent.meshData.meshPath);
    }

    void MeshManager::destroyModel(std::string& name, MeshComponent meshComponent) {
        log("Freed model id");
        m_freeModelIds.push_back(meshComponent.id);

        if(getVulkanMeshByMesh(meshComponent)->getNumOfInstances() <= 1){
            log("Erased VulkanMesh data");
            m_vulkanMeshes.erase(meshComponent.meshData.meshPath);
            for(size_t i = 0; i < meshComponent.textureNames.size(); i++){
                m_p_resources->unloadTexture(meshComponent.textureNames[i]);
            }
            log("Unloaded textures");
        }else{
            getVulkanMeshByMesh(meshComponent)->removeInstance();
        }
    }
}
