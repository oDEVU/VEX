#pragma once

#include "VulkanMesh.hpp"
#include "components/GameObjects/ModelObject.hpp"
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
        void destroyModel(ModelObject& modelObject);
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
