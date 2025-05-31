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
#include "context.hpp"
#include "SwapchainManager.hpp"
#include "Pipeline.hpp"
#include "Resources.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "VulkanMesh.hpp"

namespace vex {
    class Interface {
    private:
        VulkanContext context;
        bool shouldClose = false;

        SDL_Window *window_;
        std::unique_ptr<VulkanSwapchainManager> swapchainManager_;
        std::unique_ptr<VulkanResources> resources_;
        std::unique_ptr<VulkanPipeline> pipeline_;

        std::deque<std::unique_ptr<Model>> models_;
        std::unordered_map<std::string, Model*> modelRegistry_;

        std::vector<uint32_t> freeModelIds_;
        uint32_t nextModelId_ = 0;
        static constexpr uint32_t INVALID_MODEL_ID = UINT32_MAX;

        std::vector<std::unique_ptr<VulkanMesh>> vulkanMeshes_;

        void createDefaultTexture();
    public:
        Interface(SDL_Window* window, glm::uvec2 initialResolution);
        ~Interface();

        Model& loadModel(const std::string& modelPath, const std::string& name);
        void unloadModel(const std::string& name);
        Model* getModel(const std::string& name);

        std::chrono::high_resolution_clock::time_point startTime;
        float currentTime = 0.0f;

        void bindWindow(SDL_Window* window);
        void unbindWindow();
        void setRenderResolution(glm::uvec2 resolution);
        void renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution);
    };
}
