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

        /// @brief Creates a standard Opaque Graphics Pipeline.
        /// @details Compiles shaders, sets up input assembly, viewport, rasterizer (Cull Back), depth stencil (Write/Test), and color blending (Off/Replace).
        /// @param const std::string& vertShaderPath - Vertex shader path.
        /// @param const std::string& fragShaderPath - Fragment shader path.
        /// @param const VkVertexInputBindingDescription& bindingDescription - Vertex input setup.
        /// @param const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex attribute setup.
        void createGraphicsPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        /// @brief Creates a Transparent Graphics Pipeline.
        /// @details Sets up blending (SrcAlpha/OneMinusSrcAlpha) and disables Depth Write (keeps Depth Test).
        /// @param const std::string& vertShaderPath - Vertex shader path.
        /// @param const std::string& fragShaderPath - Fragment shader path.
        /// @param const VkVertexInputBindingDescription& bindingDescription - Vertex input setup.
        /// @param const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex attribute setup.
        void createTransparentPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        /// @brief Creates a Masked Graphics Pipeline (Alpha Cutout).
        /// @details Similar to Opaque, but typically uses a shader that discards fragments based on alpha.
        /// @param const std::string& vertShaderPath - Vertex shader path.
        /// @param const std::string& fragShaderPath - Fragment shader path.
        /// @param const VkVertexInputBindingDescription& bindingDescription - Vertex input setup.
        /// @param const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions - Vertex attribute setup.
        void createMaskedPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions
        );

        /// @brief Creates a UI Pipeline.
        /// @details Sets up blending for UI elements and ignores Depth Test/Write.
        /// @param const std::string& vertShaderPath - Vertex shader path.
        /// @param const std::string& fragShaderPath - Fragment shader path.
        /// @param const VkVertexInputBindingDescription& bindingDesc - UI vertex layout.
        /// @param const std::vector<VkVertexInputAttributeDescription>& attrDesc - UI attributes.
        void createUIPipeline(
            const std::string& vertShaderPath,
            const std::string& fragShaderPath,
            const VkVertexInputBindingDescription& bindingDesc,
            const std::vector<VkVertexInputAttributeDescription>& attrDesc);

        /// @brief Creates a Fullscreen Quad Pipeline.
        /// @details Used for post-processing or upscaling the low-res render to the swapchain.
        /// @param const std::string& vertPath - Vertex shader path.
        /// @param const std::string& fragPath - Fragment shader path.
        void createFullscreenPipeline(
            const std::string& vertPath,
            const std::string& fragPath);

        /// @brief Returns the VkPipeline handle.
        /// @return VkPipeline - The created pipeline.
        VkPipeline get() const { return m_pipeline; }

        /// @brief Returns the VkPipelineLayout handle.
        /// @return VkPipelineLayout - The pipeline layout (push constants, descriptor sets).
        VkPipelineLayout layout() const { return m_layout; }

        /// @brief Updates viewport to new resolution.
        /// @details Since dynamic state is used for Viewport/Scissor, this updates internal resolution state if needed (though mostly handled at draw time via `vkCmdSetViewport`).
        /// @param glm::uvec2 resolution - New resolution.
        void updateViewport(glm::uvec2 resolution);

        /// @brief Creates a debug VkPipeline.
        /// @param std::string& vertPath - Path to the vertex shader.
        /// @param std::string& fragPath - Path to the fragment shader.
        /// @details Creates a debug pipeline using the provided shader paths and puts some needed data in the context for later use, like layouts.
        #if DEBUG
        void createDebugPipeline(const std::string& vertPath, const std::string& fragPath);
        #endif
    private:
        VulkanContext &m_r_context;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        glm::uvec2 m_currentRenderResolution;

        /// @brief Reads a file and returns its contents as a vector of characters.
        /// @param const std::string& filename - The path to the file to read.
        /// @return std::vector<char> - The contents of the file.
        std::vector<char> readFile(const std::string& filename);
    };
}
