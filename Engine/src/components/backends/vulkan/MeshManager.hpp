#pragma once

#include "VulkanMesh.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/Mesh.hpp"
#include "entt/entity/fwd.hpp"
#include "Engine.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <entt/entt.hpp>
#include <string>

namespace vex {
    class MeshManager {
    public:
        MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources);
        ~MeshManager();

        MeshComponent loadMesh(const std::string& path);
        ModelObject* createModel(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, Engine& engine, entt::entity parent);
        void destroyModel(std::string& name, MeshComponent meshComponent);
        std::unique_ptr<VulkanMesh>& getMeshByKey(std::string& key) {
            //log("Curently %i vulkan meshes loaded.", m_vulkanMeshes.size());
            return m_vulkanMeshes.at(key);
        }

    private:
        VulkanContext& m_r_context;
        std::unique_ptr<VulkanResources>& m_p_resources;
        std::map<std::string, std::unique_ptr<VulkanMesh>> m_vulkanMeshes;
        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;
    };
}
