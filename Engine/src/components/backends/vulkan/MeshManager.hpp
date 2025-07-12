#pragma once

#include "VulkanMesh.hpp"
#include "entt/entity/fwd.hpp"

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

        entt::entity loadModel(const std::string& path, const std::string& name, entt::registry& registry, entt::entity parent);
        void unloadModel(const std::string& name, entt::registry& registry);
        std::vector<std::unique_ptr<VulkanMesh>>& getMeshes() { return m_vulkanMeshes; }
        //std::vector<std::unique_ptr<Model>>& getModels() { return m_models; }

        //Model* getModel(const std::string& name) {
        //    auto it = m_modelRegistry.find(name);
        //    return (it != m_modelRegistry.end()) ? it->second : nullptr;
        //}

    private:
        VulkanContext& m_r_context;
        std::unique_ptr<VulkanResources>& m_p_resources;
        std::vector<std::unique_ptr<VulkanMesh>> m_vulkanMeshes;
        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;
    };
}
