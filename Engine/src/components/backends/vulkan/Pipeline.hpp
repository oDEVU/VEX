#pragma once
#include "context.hpp"
#include <vector>
#include <glm/glm.hpp>
#include "components/errorUtils.hpp"

#include "components/pathUtils.hpp"
#include <filesystem>

namespace vex {
    class VulkanPipeline {
    public:
        VulkanPipeline(VulkanContext& r_context);
        ~VulkanPipeline();

        void createGraphicsPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        VkPipeline get() const { return m_pipeline; }
        VkPipelineLayout layout() const { return m_layout; }
        void updateViewport(glm::uvec2 resolution);

    private:
        VulkanContext &m_r_context;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        glm::uvec2 m_currentRenderResolution;

        std::vector<char> readFile(const std::string& filename);
    };
}
