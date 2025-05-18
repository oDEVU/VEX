#pragma once
#include "context.hpp"
#include <vector>

namespace vex {
    class VulkanPipeline {
    public:
        VulkanPipeline(VulkanContext& context);
        ~VulkanPipeline();

        void createGraphicsPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        VkPipeline get() const { return pipeline_; }
        VkPipelineLayout layout() const { return layout_; }

    private:
        VulkanContext& ctx_;
        VkPipeline pipeline_ = VK_NULL_HANDLE;
        VkPipelineLayout layout_ = VK_NULL_HANDLE;

        std::vector<char> readFile(const std::string& filename);
    };
}
