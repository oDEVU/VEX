#pragma once

#include "VulkanMesh.hpp"
#include "components/Model.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace vex {
    class MeshManager {
    public:
        MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources);
        ~MeshManager();

        Model& loadModel(const std::string& path, const std::string& name);
        void unloadModel(const std::string& name);
        Model* getModel(const std::string& name);
        std::vector<std::unique_ptr<VulkanMesh>>& getMeshes() { return m_vulkanMeshes; }
        std::vector<std::unique_ptr<Model>>& getModels() { return m_models; }

    private:
        VulkanContext& m_r_context;
        std::unique_ptr<VulkanResources>& m_p_resources;
        std::vector<std::unique_ptr<Model>> m_models;
        std::vector<std::unique_ptr<VulkanMesh>> m_vulkanMeshes;
        std::unordered_map<std::string, Model*> m_modelRegistry;
        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;
    };
}
