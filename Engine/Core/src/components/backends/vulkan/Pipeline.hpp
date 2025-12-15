/**
 *  @file   Pipeline.hpp
 *  @brief  This file defines VulkanPipeline Class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "context.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <array>
#include "components/errorUtils.hpp"

#include "components/pathUtils.hpp"
#include "components/UI/UIVertex.hpp"
#include <filesystem>

namespace vex {

    /// @brief A class managing creating vulkan pipelines. Both form meshes and UI elements.
    class VulkanPipeline {
    public:
        /// @brief Constructor for VulkanPipeline.
        /// @param VulkanContext& r_context - Reference to the VulkanContext.
        VulkanPipeline(VulkanContext& r_context);
        ~VulkanPipeline();

        /// @brief Creates a graphics VkPipeline.
        /// @param std::string& vertShaderPath - Path to the vertex shader.
        /// @param std::string& fragShaderPath - Path to the fragment shader.
        /// @param VkVertexInputBindingDescription& bindingDescription - Vertex input binding description.
        /// @param std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex input attribute descriptions.
        /// @details Creates a graphics pipeline using the provided shader paths and vertex input descriptions and puts some needed data in the context for later use, like layouts.
        void createGraphicsPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        /// @brief Creates a graphics VkPipeline.
        /// @param std::string& vertShaderPath - Path to the vertex shader.
        /// @param std::string& fragShaderPath - Path to the fragment shader.
        /// @param VkVertexInputBindingDescription& bindingDescription - Vertex input binding description.
        /// @param std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex input attribute descriptions.
        /// @details Creates a graphics pipeline using the provided shader paths and vertex input descriptions and puts some needed data in the context for later use, like layouts.
        void createTransparentPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        /// @brief Creates a UI VkPipeline.
        /// @param std::string& vertShaderPath - Path to the vertex shader.
        /// @param std::string& fragShaderPath - Path to the fragment shader.
        /// @param VkVertexInputBindingDescription& bindingDescription - Vertex input binding description.
        /// @param std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex input attribute descriptions.
        /// @details Creates a UI pipeline using the provided shader paths and vertex input descriptions and puts some needed data in the context for later use, like layouts.
        void createUIPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDesc,
            const std::vector<VkVertexInputAttributeDescription>& attrDesc);

        /// @brief Creates a fullscreen VkPipeline.
        /// @param std::string& vertPath - Path to the vertex shader.
        /// @param std::string& fragPath - Path to the fragment shader.
        /// @details Creates a fullscreen pipeline using the provided shader paths and puts some needed data in the context for later use, like layouts.
        void createFullscreenPipeline(
            const std::string& vertPath,
            const std::string& fragPath);

        /// @brief Creates a debug VkPipeline.
        /// @param std::string& vertPath - Path to the vertex shader.
        /// @param std::string& fragPath - Path to the fragment shader.
        /// @details Creates a debug pipeline using the provided shader paths and puts some needed data in the context for later use, like layouts.
        #if DEBUG
        void createDebugPipeline(const std::string& vertPath, const std::string& fragPath);
        #endif

        /// @brief Returns the VkPipeline.
        /// @return VkPipeline - The VkPipeline.
        /// @details Used by renderer (and vex UI since it manages its own rendering logic).
        VkPipeline get() const { return m_pipeline; }

        /// @brief Returns the VkPipelineLayout.
        /// @return VkPipelineLayout - The VkPipelineLayout.
        /// @details Used by renderer (and vex UI since it manages its own rendering logic).
        VkPipelineLayout layout() const { return m_layout; }

        /// @brief Updates viewport to new resolution.
        /// @param glm::uvec2 resolution - New resolution.
        /// @details Called by VulkanInterface on window resize.
        void updateViewport(glm::uvec2 resolution);

    private:
        VulkanContext &m_r_context;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        glm::uvec2 m_currentRenderResolution;

        std::vector<char> readFile(const std::string& filename);
    };
}
