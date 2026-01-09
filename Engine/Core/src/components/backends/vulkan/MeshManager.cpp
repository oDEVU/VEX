#include "MeshManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/PhysicsSystem.hpp"
#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <fstream>
#include <iterator>
#include <unordered_set>
#include <SDL3/SDL.h>
#include "limits.hpp"

#include <filesystem>
#include <algorithm>

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
        meshComponent.id = newId;
        meshComponent.textureNames.clear();

        for (const auto& texturePath : meshComponent.meshData.texturePaths) {
            meshComponent.textureNames.push_back(texturePath);
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
            log(LogLevel::ERROR, "Mesh upload failed");
            m_vulkanMeshes.erase(meshComponent.meshData.meshPath);
            handle_exception(e);
        }

        //entt::entity modelEntity = m_p_engine->getRegistry().create();
        ModelObject* modelObject = new ModelObject(*m_p_engine, tempName, meshComponent, transformComponent);
        //modelObject->cleanup = [this](std::string& tempName, const MeshComponent& meshComponent) { destroyModel(tempName, meshComponent); };
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
            log(LogLevel::ERROR, "Mesh load failed: %s", path.c_str());
            handle_exception(e);
        }

        meshComponent.meshData = std::move(meshData);

        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);

        for(auto& vertex : meshComponent.meshData.vertices) {
            min = glm::min(min, vertex.position);
            max = glm::max(max, vertex.position);
        }

        glm::vec3 center = (min + max) * 0.5f;
        float radius = glm::length(max - center);

        meshComponent.localCenter = center;
        meshComponent.localRadius = radius;

        return meshComponent;
    }

    std::unique_ptr<VulkanMesh>& MeshManager::getVulkanMeshByMesh(MeshComponent& meshComponent) {
        std::string& installedPath = m_installedPaths[meshComponent.id];
        std::string requestedPath = meshComponent.meshData.meshPath;

        if (installedPath != requestedPath) [[unlikely]] {
            log("Swapping mesh %s -> %s", installedPath.c_str(), requestedPath.c_str());

            releaseMeshReference(installedPath, meshComponent);

            installedPath = requestedPath;

            bool alreadyExisted = m_vulkanMeshes.count(requestedPath) > 0;

            registerVulkanMesh(meshComponent);

            if (alreadyExisted && m_vulkanMeshes.count(requestedPath)) {
                m_vulkanMeshes.at(requestedPath)->addInstance();
            }
        } else [[likely]] {
            registerVulkanMesh(meshComponent);
        }

        if (m_vulkanMeshes.count(requestedPath)) {
            return m_vulkanMeshes.at(requestedPath);
        }

        static std::unique_ptr<VulkanMesh> nullMesh = nullptr;
        return nullMesh;
    }

    void MeshManager::registerVulkanMesh(MeshComponent& meshComponent) {
        const std::string& path = meshComponent.meshData.meshPath;
        if (m_vulkanMeshes.count(path)) {
            if (m_meshBoundsCache.count(path)) {
                auto& bounds = m_meshBoundsCache[path];
                meshComponent.localCenter = bounds.first;
                meshComponent.localRadius = bounds.second;
                meshComponent.forceRefresh();
            }
            return;
        }
        if (!(meshComponent.meshData.meshPath == "" || meshComponent.meshData.meshPath.empty() || path == GetAssetDir())) [[unlikely]] {
            #if DEBUG
            if (meshComponent.meshData.meshPath.empty() || path == GetAssetDir() || !m_vfs->file_exists(GetAssetPath(path))) {
                log(LogLevel::WARNING, "Skipping registration for invalid path: %s", path.c_str());
                return;
            }
            #endif
            MeshComponent loadedAsset = loadMesh(path);

            meshComponent.meshData = std::move(loadedAsset.meshData);
            meshComponent.localCenter = loadedAsset.localCenter;
            meshComponent.localRadius = loadedAsset.localRadius;
            meshComponent.forceRefresh();

            m_meshBoundsCache[path] = { loadedAsset.localCenter, loadedAsset.localRadius };

            meshComponent.textureNames = std::move(loadedAsset.textureNames);
        }else [[likely]] {
            return;
        }

        meshComponent.textureNames.clear();
        std::unordered_set<std::string> uniqueTextures;

        for (const auto& texturePath : meshComponent.meshData.texturePaths) {
            if (!texturePath.empty()) {
                uniqueTextures.insert(texturePath);
                meshComponent.textureNames.push_back(texturePath);
            }
        }

        log("Lazy-loading %zu textures for %s", uniqueTextures.size(), path.c_str());

        for (const auto& texPath : uniqueTextures) {
            if (!m_p_resources->textureExists(texPath)) {
                try {
                    m_p_resources->loadTexture(texPath, texPath);
                    log("Loaded texture: %s", texPath.c_str());
                } catch (const std::exception& e) {
                    log(LogLevel::ERROR, "Failed to load texture %s", texPath.c_str());
                }
            }
        }

        try {
            log("Initializing Vulkan mesh for: %s", path.c_str());

            auto newVulkanMesh = std::make_unique<VulkanMesh>(m_r_context);
            newVulkanMesh->upload(meshComponent.meshData);
            newVulkanMesh->addInstance();

            m_vulkanMeshes.emplace(path, std::move(newVulkanMesh));

            log("Successfully registered mesh: %s", path.c_str());

        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "Failed to register VulkanMesh: %s", path.c_str());
        }
    }

    void MeshManager::onMeshComponentConstruct(entt::registry& registry, entt::entity entity) {
        auto& meshComponent = registry.get<MeshComponent>(entity);

        if (!m_freeModelIds.empty()) {
            meshComponent.id = m_freeModelIds.back();
            m_freeModelIds.pop_back();
        } else {
            meshComponent.id = m_nextModelId++;
        }

        m_installedPaths[meshComponent.id] = meshComponent.meshData.meshPath;
        const std::string& path = meshComponent.meshData.meshPath;

        bool alreadyExisted = m_vulkanMeshes.count(path) > 0;

        registerVulkanMesh(meshComponent);

        if (alreadyExisted) {
            m_vulkanMeshes.at(path)->addInstance();
        }

        if(registry.any_of<PhysicsComponent>(entity)) {
            auto& oldPC = registry.get<PhysicsComponent>(entity);
            if(oldPC.shape == ShapeType::MESH){
                PhysicsComponent newPC = PhysicsComponent::Mesh(meshComponent, oldPC.bodyType, oldPC.mass, oldPC.friction, oldPC.bounce);
                registry.replace<PhysicsComponent>(entity, newPC);
            }

        }
    }

    void MeshManager::onMeshComponentDestroy(entt::registry& registry, entt::entity entity) {
        auto& meshComponent = registry.get<MeshComponent>(entity);

        m_freeModelIds.push_back(meshComponent.id);

        if (m_vulkanMeshes.count(meshComponent.meshData.meshPath)) {
            auto& vulkanMesh = m_vulkanMeshes.at(meshComponent.meshData.meshPath);
            vulkanMesh->removeInstance();

            log("Reference count decreased for: %s (Total: %d)", meshComponent.meshData.meshPath.c_str(), vulkanMesh->getNumOfInstances());

            if (vulkanMesh->getNumOfInstances() <= 0) {
                log("Cleaning up unused mesh: %s", meshComponent.meshData.meshPath.c_str());

                for (const auto& tex : meshComponent.textureNames) {
                    m_p_resources->unloadTexture(tex);
                }

                m_vulkanMeshes.erase(meshComponent.meshData.meshPath);
            }
        }
    }

    void MeshManager::releaseMeshReference(const std::string& path, MeshComponent& ownerComp) {
        if (path.empty()) return;

        if (m_vulkanMeshes.count(path)) {
            auto& vulkanMesh = m_vulkanMeshes.at(path);
            vulkanMesh->removeInstance();

            log("Ref count decreased for: %s (Remaining: %d)", path.c_str(), vulkanMesh->getNumOfInstances());

            if (vulkanMesh->getNumOfInstances() <= 0) {
                log("Cleaning up unused mesh: %s", path.c_str());

                for (const auto& tex : ownerComp.textureNames) {
                    m_p_resources->unloadTexture(tex);
                }
                ownerComp.textureNames.clear();
                m_vulkanMeshes.erase(path);
            }
        }
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
