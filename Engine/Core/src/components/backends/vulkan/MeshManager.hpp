#pragma once

#include "VulkanMesh.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/Mesh.hpp"
#include "../../VirtualFileSystem.hpp"
#include "entt/entity/fwd.hpp"
#include "Engine.hpp"

#include <unordered_map>
#include <components/types.hpp>
#include <vector>
#include <memory>
#include <entt/entt.hpp>
#include <string>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace vex {
    class MeshManager {
    public:
        MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources, VirtualFileSystem* vfs);
        ~MeshManager();

        void init(Engine* engine){
            m_p_engine = engine;

            auto& registry = m_p_engine->getRegistry();
            registry.on_construct<MeshComponent>().connect<&MeshManager::onMeshComponentConstruct>(this);
            registry.on_destroy<MeshComponent>().connect<&MeshManager::onMeshComponentDestroy>(this);
        }

        MeshComponent loadMesh(const std::string& path);
        ModelObject* createModel(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, entt::entity parent);
        void destroyModel(std::string& name, MeshComponent meshComponent);
        void registerVulkanMesh(MeshComponent& meshComponent);
        std::unique_ptr<VulkanMesh>& getVulkanMeshByMesh(MeshComponent& meshComponent);

    private:
        VulkanContext& m_r_context;
        Engine* m_p_engine = nullptr;
        VirtualFileSystem* m_vfs;
        std::unique_ptr<VulkanResources>& m_p_resources;
        vex_map<std::string, std::unique_ptr<VulkanMesh>> m_vulkanMeshes;
        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;

        std::unordered_map<uint32_t, std::string> m_installedPaths;

        void onMeshComponentConstruct(entt::registry& registry, entt::entity entity);
        void onMeshComponentDestroy(entt::registry& registry, entt::entity entity);
        void releaseMeshReference(const std::string& path, MeshComponent& ownerComp);
    };
}
