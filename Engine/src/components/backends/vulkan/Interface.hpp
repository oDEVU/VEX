#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vector>
#include <memory>
#include <deque>
#include <glm/glm.hpp>
#include <chrono>

//VEX components
#include "components/ImGUIWrapper.hpp"
#include "context.hpp"
#include "SwapchainManager.hpp"
#include "Pipeline.hpp"
#include "Resources.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "components/GameInfo.hpp"
#include "VulkanImGUIWrapper.hpp"
#include "VulkanMesh.hpp"

namespace vex {
    class Interface {
    public:
        Interface(SDL_Window* window, glm::uvec2 initialResolution, GameInfo gInfo);
        ~Interface();

        Model& loadModel(const std::string& modelPath, const std::string& name);
        void unloadModel(const std::string& name);
        Model* getModel(const std::string& name);

        VulkanContext* getContext() { return &m_context; }

        std::chrono::high_resolution_clock::time_point startTime;
        float currentTime = 0.0f;

        void bindWindow(SDL_Window *p_window);
        void unbindWindow();
        void setRenderResolution(glm::uvec2 resolution);
        void renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution, ImGUIWrapper& m_ui, u_int64_t frame);
    private:
        VulkanContext m_context;
        bool m_shouldClose = false;

        SDL_Window *m_p_window;
        std::unique_ptr<VulkanSwapchainManager> m_p_swapchainManager;
        std::unique_ptr<VulkanResources> m_p_resources;
        std::unique_ptr<VulkanPipeline> m_p_pipeline;

        std::deque<std::unique_ptr<Model>> m_models;
        std::unordered_map<std::string, Model*> m_modelRegistry;

        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;
        static constexpr uint32_t INVALID_MODEL_ID = UINT32_MAX;

        std::vector<std::unique_ptr<VulkanMesh>> m_vulkanMeshes;

        void createDefaultTexture();
        void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                            VkImageLayout oldLayout, VkImageLayout newLayout,
                                            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
    };
}
