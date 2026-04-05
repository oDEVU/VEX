#include "Renderer.hpp"
#include "components/backends/vulkan/Pipeline.hpp"
#include "components/backends/vulkan/uniforms.hpp"
#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <memory>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <components/GameComponents/BasicComponents.hpp>
#include "frustum.hpp"
#include "limits.hpp"
#include "components/HardwareInfo.hpp"
#include <immintrin.h>

#if defined(max)
    #undef max
#endif

namespace vex {
    struct FrustumSoA {
        __m256 m_nx, m_ny, m_nz, m_dist;

        void init(const vex::Frustum& f) {
            m_nx = _mm256_setr_ps(f.planes[0].normal.x, f.planes[1].normal.x, f.planes[2].normal.x, f.planes[3].normal.x, f.planes[4].normal.x, f.planes[5].normal.x, 0.0f, 0.0f);
            m_ny = _mm256_setr_ps(f.planes[0].normal.y, f.planes[1].normal.y, f.planes[2].normal.y, f.planes[3].normal.y, f.planes[4].normal.y, f.planes[5].normal.y, 0.0f, 0.0f);
            m_nz = _mm256_setr_ps(f.planes[0].normal.z, f.planes[1].normal.z, f.planes[2].normal.z, f.planes[3].normal.z, f.planes[4].normal.z, f.planes[5].normal.z, 0.0f, 0.0f);
            m_dist = _mm256_setr_ps(f.planes[0].distance, f.planes[1].distance, f.planes[2].distance, f.planes[3].distance, f.planes[4].distance, f.planes[5].distance, 0.0f, 0.0f);
        }

        __attribute__((target("avx2")))
        bool testSphereAVX(const glm::vec3& center, float radius) const {
            __m256 cx = _mm256_set1_ps(center.x);
            __m256 cy = _mm256_set1_ps(center.y);
            __m256 cz = _mm256_set1_ps(center.z);
            __m256 r  = _mm256_set1_ps(-radius);

            __m256 dot = _mm256_fmadd_ps(m_nx, cx, m_dist);
            dot = _mm256_fmadd_ps(m_ny, cy, dot);
            dot = _mm256_fmadd_ps(m_nz, cz, dot);
            __m256 mask = _mm256_cmp_ps(dot, r, _CMP_LT_OQ);

            int res = _mm256_movemask_ps(mask);

            return (res & 0x3F) == 0;
        }
    };

    glm::vec3 extractCameraPosition(const glm::mat4& view) {
        glm::mat4 invView = glm::inverse(view);
        return glm::vec3(invView[3]);
    }

    Renderer::Renderer(VulkanContext& context,
             std::unique_ptr<VulkanResources>& resources,
             std::unique_ptr<VulkanPipeline>& pipeline,
             std::unique_ptr<VulkanPipeline>& transPipeline,
             std::unique_ptr<VulkanPipeline>& maskPipeline,
             std::unique_ptr<VulkanPipeline>& billboardTransPipeline,
             std::unique_ptr<VulkanPipeline>& billboardMaskedPipeline,
             std::unique_ptr<VulkanPipeline>& particleTransPipeline,
             std::unique_ptr<VulkanPipeline>& particleMaskedPipeline,
             std::unique_ptr<VulkanPipeline>& uiPipeline,
             std::unique_ptr<VulkanPipeline>& fullscreenPipeline,
             std::unique_ptr<VulkanPipeline>& compositePipeline,
             std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
             std::unique_ptr<MeshManager>& meshManager)
        : m_r_context(context), m_p_resources(resources),
          m_p_pipeline(pipeline), m_p_transPipeline(transPipeline),
          m_p_maskPipeline(maskPipeline),
          m_p_billboardTransPipeline(billboardTransPipeline),
          m_p_billboardMaskedPipeline(billboardMaskedPipeline),
          m_p_particleTransPipeline(particleTransPipeline),
          m_p_particleMaskedPipeline(particleMaskedPipeline),
          m_p_uiPipeline(uiPipeline),
          m_p_fullscreenPipeline(fullscreenPipeline),
          m_p_compositePipeline(compositePipeline),
          m_p_swapchainManager(swapchainManager),
          m_p_meshManager(meshManager) {
        startTime = std::chrono::high_resolution_clock::now();

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_NEAREST;
                samplerInfo.minFilter = VK_FILTER_NEAREST;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.maxAnisotropy = 1.0f;

                if (vkCreateSampler(m_r_context.device, &samplerInfo, nullptr, &m_screenSampler) != VK_SUCCESS) {
                    throw_error("Failed to create screen sampler");
                }

                VkSamplerCreateInfo linearSamplerInfo = samplerInfo;
                linearSamplerInfo.magFilter = VK_FILTER_LINEAR;
                linearSamplerInfo.minFilter = VK_FILTER_LINEAR;
                linearSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                linearSamplerInfo.minLod = 0.0f;
                linearSamplerInfo.maxLod = 10.0f;

                if (vkCreateSampler(m_r_context.device, &linearSamplerInfo, nullptr, &m_linearSampler) != VK_SUCCESS) {
                    throw_error("Failed to create linear sampler");
                }

                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

                VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12 };
                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                poolInfo.maxSets = 2;

                vkCreateDescriptorPool(m_r_context.device, &poolInfo, nullptr, &m_localPool);

                allocInfo.descriptorPool = m_localPool;
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &m_r_context.screenDescriptorSetLayout;

                vkAllocateDescriptorSets(m_r_context.device, &allocInfo, &m_screenDescriptorSet);
                allocInfo.pSetLayouts = &m_r_context.screenDescriptorSetLayout;
                vkAllocateDescriptorSets(m_r_context.device, &allocInfo, &m_crtDescriptorSet);

                #if DEBUG
                    m_editorCameraVulkanMesh = std::make_unique<VulkanMesh>(m_r_context);

                    m_debugBuffers.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
                    m_debugAllocations.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

                    VkBufferCreateInfo debugBufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
                    debugBufInfo.size = 1024 * 1024;
                    debugBufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

                    VmaAllocationCreateInfo debugAllocInfo{};
                    debugAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

                    for(size_t i=0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
                        vmaCreateBuffer(m_r_context.allocator, &debugBufInfo, &debugAllocInfo, &m_debugBuffers[i], &m_debugAllocations[i], nullptr);
                    }
                #endif
                m_garbageDescriptors.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

                if (m_r_context.supportsIndirectDraw) {
                        m_indirectBuffers.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
                        m_indirectAllocations.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

                        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                        bufferInfo.size = 10000 * sizeof(VkDrawIndexedIndirectCommand);
                        bufferInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                        VmaAllocationCreateInfo allocInfo = {};
                        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

                        for(size_t i=0; i<m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
                            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo, &m_indirectBuffers[i], &m_indirectAllocations[i], nullptr);
                        }
                    }

        log("Renderer initialized successfully");
    }

    Renderer::~Renderer() {
        if (m_screenSampler) vkDestroySampler(m_r_context.device, m_screenSampler, nullptr);
        if (m_linearSampler) vkDestroySampler(m_r_context.device, m_linearSampler, nullptr);
        if (m_localPool) vkDestroyDescriptorPool(m_r_context.device, m_localPool, nullptr);

        #if DEBUG
            for(size_t i=0; i < m_debugBuffers.size(); i++) {
                if(m_debugBuffers[i]) vmaDestroyBuffer(m_r_context.allocator, m_debugBuffers[i], m_debugAllocations[i]);
            }
        #endif

        if (!m_indirectBuffers.empty()) {
            for(size_t i=0; i < m_indirectBuffers.size(); i++) {
                if(m_indirectBuffers[i] != VK_NULL_HANDLE) {
                    vmaDestroyBuffer(m_r_context.allocator, m_indirectBuffers[i], m_indirectAllocations[i]);
                }
            }
            m_indirectBuffers.clear();
            m_indirectAllocations.clear();
        }

        log("Renderer destroyed");
    }

    #if DEBUG
    void Renderer::renderDebug(VkCommandBuffer cmd, int frameIndex, const std::vector<DebugVertex>& lines) {
        if(lines.empty() || !m_pp_debugPipeline) return;

        void* mappedData;
        vmaMapMemory(m_r_context.allocator, m_debugAllocations[frameIndex], &mappedData);
        size_t dataSize = lines.size() * sizeof(DebugVertex);
        if(dataSize > 1024 * 1024) dataSize = 1024 * 1024;
        memcpy(mappedData, lines.data(), dataSize);
        vmaUnmapMemory(m_r_context.allocator, m_debugAllocations[frameIndex]);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (*m_pp_debugPipeline)->get());
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_r_context.currentRenderResolution.x);
        viewport.height = static_cast<float>(m_r_context.currentRenderResolution.y);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        VkDescriptorSet sceneSet = m_p_resources->getUBODescriptorSet(frameIndex);

        uint32_t dynamicOffset = 0;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (*m_pp_debugPipeline)->layout(), 0, 1, &sceneSet, 1, &dynamicOffset);

        VkBuffer vBuffers[] = { m_debugBuffers[frameIndex] };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, offsets);

        vkCmdDraw(cmd, static_cast<uint32_t>(lines.size()), 1, 0, 0);
    }
    #endif

    bool Renderer::beginFrame(glm::uvec2 renderResolution, SceneRenderData& outData) {
        try {
            if (renderResolution.x == 0 || renderResolution.y == 0 ||
                renderResolution.x > 32768 || renderResolution.y > 32768) {
                return false;
            }

            if (renderResolution != m_r_context.currentRenderResolution || m_r_context.requestSwapchainRecreation) {
                log(LogLevel::INFO, "Recreating Swapchain", m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y, renderResolution.x, renderResolution.y);
                m_r_context.currentRenderResolution = renderResolution;
                m_p_pipeline->updateViewport(renderResolution);
                m_p_swapchainManager->recreateSwapchain();
                m_lastUsedView = VK_NULL_HANDLE;
                if (m_cachedImGuiDescriptor != VK_NULL_HANDLE) {
                    m_garbageDescriptors[m_r_context.currentFrame].push_back(m_cachedImGuiDescriptor);
                }
                m_cachedImGuiDescriptor = VK_NULL_HANDLE;
                m_lastUsedView = m_r_context.lowResColorView;
                updateScreenDescriptor(m_r_context.lowResColorView);
            }

            vkWaitForFences(m_r_context.device, 1, &m_r_context.inFlightFences[m_r_context.currentFrame], VK_TRUE, UINT64_MAX);

            VkResult result = vkAcquireNextImageKHR(
                m_r_context.device,
                m_r_context.swapchain,
                1000000000,
                m_r_context.imageAvailableSemaphores[m_r_context.currentFrame],
                VK_NULL_HANDLE,
                &m_r_context.currentImageIndex
            );

            if (result == VK_TIMEOUT) {
                m_r_context.requestSwapchainRecreation = true;
                outData.isSwapchainValid = false;
                return false;
            }

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                m_p_swapchainManager->recreateSwapchain();
                outData.isSwapchainValid = false;
                log(LogLevel::WARNING, "Swapchain out of date");
                return false;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw_error("Failed to acquire swap chain image!");
            }

            vkResetFences(m_r_context.device, 1, &m_r_context.inFlightFences[m_r_context.currentFrame]);
            vkResetCommandPool(m_r_context.device, m_r_context.commandPools[m_r_context.currentFrame], 0);

            outData.commandBuffer = m_r_context.commandBuffers[m_r_context.currentFrame];
            outData.frameIndex = m_r_context.currentFrame;
            outData.imageIndex = m_r_context.currentImageIndex;
            outData.isSwapchainValid = true;

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(outData.commandBuffer, &beginInfo);

            return true;
        } catch (const std::exception& e) {
            log(LogLevel::ERROR, "beginFrame failed");
            handle_critical_exception(e);
            return false;
        }
    }

        void Renderer::renderScene(SceneRenderData& data, const entt::entity cameraEntity, entt::registry& registry, int frame, const std::vector<DebugVertex>* debugLines, bool isEditorMode) {
            VkCommandBuffer cmd = data.commandBuffer;

            auto now = std::chrono::high_resolution_clock::now();
            currentTime = std::chrono::duration<float>(now - startTime).count();

            if (m_r_context.supportsBindlessTextures) {
                    VkDescriptorSet globalSet = m_p_resources->getBindlessDescriptorSet();
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->layout(),
                                            1, 1, &globalSet, 0, nullptr);
                }

            if (m_lastUsedView != m_r_context.lowResColorView) {
                updateScreenDescriptor(m_r_context.lowResColorView);
                if (m_cachedImGuiDescriptor != VK_NULL_HANDLE) {
                    m_garbageDescriptors[m_r_context.currentFrame].push_back(m_cachedImGuiDescriptor);
                }
                m_cachedImGuiDescriptor = VK_NULL_HANDLE;
                m_lastUsedView = m_r_context.lowResColorView;
            }

            glm::vec3 finalClearColor = m_r_context.m_environment.clearColor;
            auto fogView = registry.view<FogComponent>();

            for (auto entity : fogView) {
                auto& fc = fogView.get<FogComponent>(entity);
                m_sceneUBO.fogColor = glm::vec4(fc.color, fc.density);
                m_sceneUBO.fogDistances = glm::vec2(fc.start, fc.end);

                float skyMixFactor = glm::clamp(fc.density, 0.0f, 1.0f);
                finalClearColor = glm::mix(finalClearColor, fc.color, skyMixFactor);

                break;
            }

            auto modelView = registry.view<TransformComponent, MeshComponent>();
            std::vector<MeshComponent*> pendingMeshes;

            for (auto entity : modelView) {
                auto& mesh = modelView.get<MeshComponent>(entity);
                const std::string& path = mesh.meshData.meshPath;

                if (!path.empty() && !m_p_meshManager->isMeshLoaded(path)) {
                    bool found = false;
                    for (auto* m : pendingMeshes) {
                        if (m->meshData.meshPath == path) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        pendingMeshes.push_back(&mesh);
                    }
                }
            }

            if (!pendingMeshes.empty()) {
                m_p_meshManager->loadMeshesAsync(pendingMeshes);
            }

            transitionImageLayout(cmd,
                                m_r_context.lowResColorImage,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                0,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            transitionImageLayout(cmd,
                                m_r_context.depthImage,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                0,
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

            VkRenderingAttachmentInfo colorAttachment{};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachment.imageView = m_r_context.lowResColorView;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue.color = {{finalClearColor.x, finalClearColor.y, finalClearColor.z, 1.0f}};

            VkRenderingAttachmentInfo depthAttachment{};
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = m_r_context.depthImageView;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil = {1.0f, 0};

            VkRenderingInfo renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderingInfo.renderArea.offset = {0, 0};
            renderingInfo.renderArea.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y};
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthAttachment;

            try {
                vkCmdBeginRendering(cmd, &renderingInfo);
            } catch (const std::exception& e) {
                handle_critical_exception(e);
                return;
            }

            VkViewport viewport{};
            viewport.width = (float)m_r_context.currentRenderResolution.x;
            viewport.height = (float)m_r_context.currentRenderResolution.y;
            viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            //std::cout << "textureView x:" << viewport.width << ", y:" << viewport.height << std::endl;

            VkRect2D scissor{};
            scissor.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            glm::mat4 view = glm::mat4(1.0f);
            glm::mat4 proj = glm::mat4(1.0f);
            auto& transform = registry.get<TransformComponent>(cameraEntity);
            auto& camera = registry.get<CameraComponent>(cameraEntity);

            if(!transform.isReady()){
                transform.setRegistry(registry);
            }

            transform.recalculateMatrix();

            view = glm::lookAt(transform.getWorldPosition(), transform.getWorldPosition() + transform.getForwardVector(), transform.getUpVector());
            proj = glm::perspective(glm::radians(camera.fov), (float)m_r_context.currentRenderResolution.x / (float)m_r_context.currentRenderResolution.y, camera.nearPlane, camera.farPlane);
            proj[1][1] *= -1;

            //log("Updating scene UBO...");
            m_sceneUBO.view = view;
            m_sceneUBO.proj = proj;

            m_sceneUBO.snapResolution = 1.f;
            m_sceneUBO.jitterIntensity = 0.5f;

            m_sceneUBO.enablePS1Effects = 0;

            if(m_r_context.m_environment.vertexSnapping){
                m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_SNAPPING;
            }

            if(m_r_context.m_environment.passiveVertexJitter){
                m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_JITTER;
            }

            if(m_r_context.m_environment.affineWarping){
                m_sceneUBO.enablePS1Effects |= PS1Effects::AFFINE_WARPING;
            }

            if(m_r_context.m_environment.screenQuantization){
                m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_QUANTIZATION;
            }

            if(m_r_context.m_environment.ntfsArtifacts){
                m_sceneUBO.enablePS1Effects |= PS1Effects::NTSC_ARTIFACTS;
            }

            if(m_r_context.m_environment.gourardShading){
                m_sceneUBO.enablePS1Effects |= PS1Effects::GOURAUD_SHADING;
            }

            if(m_r_context.m_environment.textureQuantization){
                m_sceneUBO.enablePS1Effects |= PS1Effects::TEXTURE_QUANTIZATION;
            }

            if(m_r_context.m_environment.screenDither){
                m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_DITHER;
            }

            m_sceneUBO.renderResolution = m_r_context.currentRenderResolution;
            m_sceneUBO.windowResolution = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
            m_sceneUBO.time = currentTime;
            m_sceneUBO.frame = frame;
            m_sceneUBO.upscaleRatio = m_r_context.swapchainExtent.height / static_cast<float>(m_r_context.currentRenderResolution.y);

            m_sceneUBO.ambientLight = glm::vec4(m_r_context.m_environment.ambientLight,1.0f);
            m_sceneUBO.ambientLightStrength = m_r_context.m_environment.ambientLightStrength;
            m_sceneUBO.sunLight = glm::vec4(m_r_context.m_environment.sunLight,1.0f);
            m_sceneUBO.sunDirection = glm::vec4(m_r_context.m_environment.sunDirection,1.0f);

            m_p_resources->updateSceneUBO(m_sceneUBO);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());

            glm::vec3 cameraPos = extractCameraPosition(view);
            Frustum camFrustum;
            camFrustum.update(proj * view);

            m_transparentTriangles.clear();
            trnasMatrixes.clear();
            uint32_t modelIndex = 0;

            opaqueQueue.clear();
            maskedQueue.clear();
            transparentQueue.clear();
            bMaskedQueue.clear();
            bTransQueue.clear();
            pMaskedQueue.clear();
            pTransQueue.clear();

            FrustumSoA frustumSimd;
            static bool useAVX = HardwareInfo::HasAVX2();
            if (useAVX) {
                frustumSimd.init(camFrustum);
            }

            //auto modelView = registry.view<TransformComponent, MeshComponent>();
            auto it = modelView.begin();
            auto end = modelView.end();

            for (; it != end; ++it) {
                const auto entity = *it;

                auto nextIt = it;
                ++nextIt;
                if (nextIt != end) {
                    const auto nextEntity = *nextIt;
                    if(const auto* nextMesh = registry.try_get<MeshComponent>(nextEntity)) {
                        _mm_prefetch(reinterpret_cast<const char*>(nextMesh), _MM_HINT_T0);
                    }
                    if(const auto* nextTrans = registry.try_get<TransformComponent>(nextEntity)) {
                        _mm_prefetch(reinterpret_cast<const char*>(nextTrans), _MM_HINT_T0);
                    }
                }

                auto& transform = modelView.get<TransformComponent>(entity);
                auto& mesh = modelView.get<MeshComponent>(entity);
                bool boundsNeedUpdate = transform.isDirty() || mesh.getIsFresh() || isEditorMode || mesh.worldRadius <= 0.0f;
                glm::mat4 modelMatrix = transform.matrix();

                if(!transform.isReady()){
                    transform.setRegistry(registry);
                }

                if(boundsNeedUpdate){
                    float scaleX = glm::length(glm::vec3(modelMatrix[0]));
                    float scaleY = glm::length(glm::vec3(modelMatrix[1]));
                    float scaleZ = glm::length(glm::vec3(modelMatrix[2]));

                    float maxScale = std::max({ scaleX, scaleY, scaleZ });

                    mesh.worldRadius = mesh.localRadius * maxScale;
                    mesh.worldCenter = (modelMatrix * glm::vec4(mesh.localCenter, 1.0f));

                    #if DEBUG
                    if(isEditorMode && frame > 0){
                        transform.convertRot();
                    }else if(isEditorMode){
                        transform.rotation = transform.getLocalRotation();
                    }
                    transform.rotation = glm::mod(transform.rotation, glm::vec3(360.0f));
                    #endif
                }

                if(!mesh.getIsFresh()) [[unlikely]]{
                    bool visible;
                    if (useAVX) [[likely]] {
                        visible = frustumSimd.testSphereAVX(mesh.worldCenter, mesh.worldRadius);
                    } else {
                        visible = camFrustum.testSphere(mesh.worldCenter, mesh.worldRadius);
                    }

                    if (!visible) continue;
                }

                auto lightView = registry.view<TransformComponent, LightComponent>();

                SceneLightsUBO lightUBO;
                lightUBO.lightCount = 0;

                for(auto lightEntity : lightView){

                    if (lightUBO.lightCount >= MAX_DYNAMIC_LIGHTS) {
                        lightUBO.lightCount = MAX_DYNAMIC_LIGHTS;
                        log(LogLevel::WARNING, "Limit reached of per object dynamic lights, rest of the light wont affect this mesh.");
                        break;
                    }

                    auto& light = lightView.get<LightComponent>(lightEntity);
                    auto& transform = lightView.get<TransformComponent>(lightEntity);

                    if(!transform.isReady()){
                        transform.setRegistry(registry);
                    }

                    float dist = glm::distance(transform.getWorldPosition(), mesh.worldCenter);
                    if (dist < (light.radius + mesh.worldRadius)) {
                        auto& targetLight = lightUBO.lights[lightUBO.lightCount];
                        targetLight.position = glm::vec4(transform.getWorldPosition(), light.radius);
                        targetLight.color = glm::vec4(light.color, light.intensity);
                        lightUBO.lightCount++;
                    }
                }
                m_p_resources->updateLightUBO(m_r_context.currentFrame, modelIndex, lightUBO);

                if (mesh.renderType == RenderType::OPAQUE) {
                    opaqueQueue.push_back({entity, modelIndex});
                    /* auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);
                    if (vulkanMesh) {
                        vulkanMesh->draw(cmd, m_p_pipeline->layout(), *m_p_resources, data.frameIndex, modelIndex, modelMatrix, mesh.color);
                    } */
                } else if (mesh.renderType == RenderType::MASKED) {
                    maskedQueue.push_back({entity, modelIndex});
                    /* auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);
                    if (vulkanMesh) {
                        vulkanMesh->draw(cmd, m_p_maskPipeline->layout(), *m_p_resources, data.frameIndex, modelIndex, modelMatrix, mesh.color);
                    } */
                } else if (mesh.renderType == RenderType::TRANSPARENT) {
                    transparentQueue.push_back({entity, modelIndex});
                }
                modelIndex++;
                if(mesh.getIsFresh()) mesh.setRendered();
            }

            auto bView = registry.view<TransformComponent, BillboardComponent>();
            for (auto entity : bView) {
                auto& bill = bView.get<BillboardComponent>(entity);
                uint32_t tIndex = m_p_resources->getTextureIndex(GetAssetPath(bill.texturePath));
                if (tIndex == 0) tIndex = m_p_resources->getTextureIndex("default");

                if (bill.isTransparent) bTransQueue.push_back({entity, tIndex});
                else bMaskedQueue.push_back({entity, tIndex});
            }

            auto pView = registry.view<ParticleEmitterComponent>();
            for (auto entity : pView) {
                auto& emit = pView.get<ParticleEmitterComponent>(entity);
                if (emit.activeParticles.empty()) continue;

                if (emit.isTransparent) pTransQueue.push_back(entity);
                else pMaskedQueue.push_back(entity);
            }

            for (const auto& item : opaqueQueue) {
                auto& mesh = registry.get<MeshComponent>(item.entity);
                auto& transform = registry.get<TransformComponent>(item.entity);
                auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);

                if (vulkanMesh) {
                    vulkanMesh->draw(cmd, m_p_pipeline->layout(), *m_p_resources, data.frameIndex, item.modelIndex, transform.matrix(), mesh);
                }
            }

            #if DEBUG
            if(isEditorMode){
                auto modelView = registry.view<TransformComponent, CameraComponent>();
                for (auto entity : modelView) {
                    if(cameraEntity == entity){
                        break;
                    }
                    auto& transform = modelView.get<TransformComponent>(entity);
                    glm::vec3 worldScale = transform.getWorldScale();
                    transform.setWorldScale(glm::vec3(1.f));
                    if(m_editorCameraVulkanMesh->getNumOfInstances() <= 0){
                        m_editorCameraMesh.loadFromRawFile("../Assets/meshes/editorCamera.obj");
                        m_editorCameraVulkanMesh->upload(m_editorCameraMesh);
                        m_editorCameraVulkanMesh->addInstance();
                    }else{
                        auto mc = MeshComponent{};
                        mc.color = glm::vec4(0.3f, 1.0f, 0.5f, 1.0f);
                        m_editorCameraVulkanMesh->draw(cmd, m_p_pipeline->layout(), *m_p_resources, data.frameIndex, 0, transform.matrix(), mc);
                    }
                    transform.setWorldScale(worldScale);
                }
            }
            #endif

            if (!maskedQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_maskPipeline->get());

                for (const auto& item : maskedQueue) {
                    auto& mesh = registry.get<MeshComponent>(item.entity);
                    auto& transform = registry.get<TransformComponent>(item.entity);
                    auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);

                    if (vulkanMesh) {
                        vulkanMesh->draw(cmd, m_p_maskPipeline->layout(), *m_p_resources, data.frameIndex, item.modelIndex, transform.matrix(), mesh);
                    }
                }
            }

            uint32_t currentParticleOffset = 0;
            ParticleGPUData* particleMappedData = static_cast<ParticleGPUData*>(m_p_resources->getParticleMappedData(data.frameIndex));
            VkDescriptorSet particleSSBODescriptorSet = m_p_resources->getParticleDescriptorSet(data.frameIndex);

            if (!bMaskedQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->get());
                VkViewport viewport{0.0f, 0.0f, (float)m_r_context.currentRenderResolution.x, (float)m_r_context.currentRenderResolution.y, 0.0f, 1.0f};
                VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);
                VkDescriptorSet globalSet_bMaskedQueue = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset_bMaskedQueue = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->layout(), 0, 1, &globalSet_bMaskedQueue, 1, &dynamicOffset_bMaskedQueue);
                if (m_r_context.supportsBindlessTextures) {
                    VkDescriptorSet bindlessSet = m_p_resources->getBindlessDescriptorSet();
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->layout(), 1, 1, &bindlessSet, 0, nullptr);
                }
                for (const auto& item : bMaskedQueue) {
                    auto& trans = registry.get<TransformComponent>(item.entity);
                    auto& bill = registry.get<BillboardComponent>(item.entity);

                    BillboardPushData push{};
                    push.pos = trans.getWorldPosition();
                    push.sx = bill.size.x;
                    push.sy = bill.size.y;
                    push.tID = item.texID;
                    push.unlit = bill.isUnlit ? 1 : 0;
                    push.col = glm::vec4(bill.color.r, bill.color.g, bill.color.b, bill.color.a);

                    vkCmdPushConstants(cmd, m_p_billboardMaskedPipeline->layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BillboardPushData), &push);

                    VkDescriptorSet globalSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                    uint32_t dynamicOffset = 0;
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->layout(), 0, 1, &globalSet, 1, &dynamicOffset);

                    if (m_r_context.supportsBindlessTextures) {
                        VkDescriptorSet bindlessSet = m_p_resources->getBindlessDescriptorSet();
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->layout(), 1, 1, &bindlessSet, 0, nullptr);
                    } else {
                        VkDescriptorSet texSet = m_p_resources->getTextureDescriptorSet(data.frameIndex, item.texID);
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardMaskedPipeline->layout(), 1, 1, &texSet, 0, nullptr);
                    }

                    vkCmdDraw(cmd, 6, 1, 0, 0);
                }
            }

            if (!pMaskedQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleMaskedPipeline->get());
                VkViewport viewport{0.0f, 0.0f, (float)m_r_context.currentRenderResolution.x, (float)m_r_context.currentRenderResolution.y, 0.0f, 1.0f};
                VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                VkDescriptorSet globalSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleMaskedPipeline->layout(), 0, 1, &globalSet, 1, &dynamicOffset);

                if (m_r_context.supportsBindlessTextures) {
                    VkDescriptorSet bindlessSet = m_p_resources->getBindlessDescriptorSet();
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleMaskedPipeline->layout(), 1, 1, &bindlessSet, 0, nullptr);
                }

                for (auto e : pMaskedQueue) {
                    auto& emit = registry.get<ParticleEmitterComponent>(e);
                    uint32_t pCount = static_cast<uint32_t>(emit.activeParticles.size());

                    if (currentParticleOffset + pCount > 100000) {
                        vex::log(LogLevel::WARNING, "Max particle limit reached! Skipping further particles.");
                        continue;
                    }




                    uint32_t tIndex = m_p_resources->getTextureIndex(GetAssetPath(emit.texturePath));
                    if (tIndex == 0) tIndex = m_p_resources->getTextureIndex("default");
                    for(auto& p : emit.activeParticles) p.textureID = tIndex;

                    memcpy(particleMappedData + currentParticleOffset, emit.activeParticles.data(), pCount * sizeof(ParticleGPUData));

                    if (!m_r_context.supportsBindlessTextures) {
                        VkDescriptorSet texSet = m_p_resources->getTextureDescriptorSet(data.frameIndex, tIndex);
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleMaskedPipeline->layout(), 1, 1, &texSet, 0, nullptr);
                    }

                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleMaskedPipeline->layout(), 2, 1, &particleSSBODescriptorSet, 0, nullptr);

                    vkCmdDraw(cmd, 6, pCount, 0, currentParticleOffset);
                    currentParticleOffset += pCount;
                }
            }

            vkCmdEndRendering(cmd);

                transitionImageLayout(cmd, m_r_context.accumImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                transitionImageLayout(cmd, m_r_context.revealImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

                VkRenderingAttachmentInfo transAttachments[2]{};

                transAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                transAttachments[0].imageView = m_r_context.accumView;
                transAttachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                transAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                transAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                transAttachments[0].clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

                transAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                transAttachments[1].imageView = m_r_context.revealView;
                transAttachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                transAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                transAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                transAttachments[1].clearValue.color = {{1.0f, 0.0f, 0.0f, 0.0f}};

                VkRenderingAttachmentInfo transDepthAttachment{};
                transDepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                transDepthAttachment.imageView = m_r_context.depthImageView;
                transDepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                transDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                transDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_NONE;

                VkRenderingInfo transRenderingInfo{};
                transRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                transRenderingInfo.renderArea.offset = {0, 0};
                transRenderingInfo.renderArea.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y};
                transRenderingInfo.layerCount = 1;
                transRenderingInfo.colorAttachmentCount = 2;
                transRenderingInfo.pColorAttachments = transAttachments;
                transRenderingInfo.pDepthAttachment = &transDepthAttachment;

                vkCmdBeginRendering(cmd, &transRenderingInfo);

            if (!transparentQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_transPipeline->get());

                for (const auto& item : transparentQueue) {
                    auto& mesh = registry.get<MeshComponent>(item.entity);
                    auto& transform = registry.get<TransformComponent>(item.entity);
                    auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);

                    if (vulkanMesh) {
                        vulkanMesh->draw(cmd, m_p_transPipeline->layout(), *m_p_resources, data.frameIndex, item.modelIndex, transform.matrix(), mesh);
                    }
                }
            }

            if (!bTransQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardTransPipeline->get());
                VkViewport viewport{0.0f, 0.0f, (float)m_r_context.currentRenderResolution.x, (float)m_r_context.currentRenderResolution.y, 0.0f, 1.0f};
                VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);
                VkDescriptorSet globalSet_bTransQueue = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset_bTransQueue = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardTransPipeline->layout(), 0, 1, &globalSet_bTransQueue, 1, &dynamicOffset_bTransQueue);
                if (m_r_context.supportsBindlessTextures) {
                    VkDescriptorSet bindlessSet = m_p_resources->getBindlessDescriptorSet();
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardTransPipeline->layout(), 1, 1, &bindlessSet, 0, nullptr);
                }
                for (const auto& item : bTransQueue) {
                    auto& trans = registry.get<TransformComponent>(item.entity);
                    auto& bill = registry.get<BillboardComponent>(item.entity);

                    BillboardPushData push{};
                    push.pos = trans.getWorldPosition();
                    push.sx = bill.size.x;
                    push.sy = bill.size.y;
                    push.tID = item.texID;
                    push.unlit = bill.isUnlit ? 1 : 0;
                    push.col = glm::vec4(bill.color.r, bill.color.g, bill.color.b, bill.color.a);

                    vkCmdPushConstants(cmd, m_p_billboardTransPipeline->layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BillboardPushData), &push);



                    if (!m_r_context.supportsBindlessTextures) {
                        VkDescriptorSet texSet = m_p_resources->getTextureDescriptorSet(data.frameIndex, item.texID);
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_billboardTransPipeline->layout(), 1, 1, &texSet, 0, nullptr);
                    }

                    vkCmdDraw(cmd, 6, 1, 0, 0);
                }
            }

            if (!pTransQueue.empty()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleTransPipeline->get());
                VkViewport viewport{0.0f, 0.0f, (float)m_r_context.currentRenderResolution.x, (float)m_r_context.currentRenderResolution.y, 0.0f, 1.0f};
                VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                VkDescriptorSet globalSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleTransPipeline->layout(), 0, 1, &globalSet, 1, &dynamicOffset);

                if (m_r_context.supportsBindlessTextures) {
                    VkDescriptorSet bindlessSet = m_p_resources->getBindlessDescriptorSet();
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleTransPipeline->layout(), 1, 1, &bindlessSet, 0, nullptr);
                }

                for (auto e : pTransQueue) {
                    auto& emit = registry.get<ParticleEmitterComponent>(e);
                    uint32_t pCount = static_cast<uint32_t>(emit.activeParticles.size());

                    if (currentParticleOffset + pCount > 100000) {
                        vex::log(LogLevel::WARNING, "Max particle limit reached! Skipping further particles.");
                        continue;
                    }




                    uint32_t tIndex = m_p_resources->getTextureIndex(GetAssetPath(emit.texturePath));
                    if (tIndex == 0) tIndex = m_p_resources->getTextureIndex("default");
                    for(auto& p : emit.activeParticles) p.textureID = tIndex;

                    memcpy(particleMappedData + currentParticleOffset, emit.activeParticles.data(), pCount * sizeof(ParticleGPUData));

                    if (!m_r_context.supportsBindlessTextures) {
                        VkDescriptorSet texSet = m_p_resources->getTextureDescriptorSet(data.frameIndex, tIndex);
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleTransPipeline->layout(), 1, 1, &texSet, 0, nullptr);
                    }

                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_particleTransPipeline->layout(), 2, 1, &particleSSBODescriptorSet, 0, nullptr);

                    vkCmdDraw(cmd, 6, pCount, 0, currentParticleOffset);
                    currentParticleOffset += pCount;
                }
            }

            vkCmdEndRendering(cmd);

            transitionImageLayout(cmd, m_r_context.accumImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            transitionImageLayout(cmd, m_r_context.revealImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            transitionImageLayout(cmd, m_r_context.uiImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkRenderingAttachmentInfo uiColorAttachment = colorAttachment;
            uiColorAttachment.imageView = m_r_context.uiView;
            uiColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            uiColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

            VkRenderingAttachmentInfo uiDepthAttachment = depthAttachment;
            uiDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            uiDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_NONE;

            VkRenderingInfo uiRenderingInfo = renderingInfo;
            uiRenderingInfo.pColorAttachments = &uiColorAttachment;
            uiRenderingInfo.pDepthAttachment = &uiDepthAttachment;

            vkCmdBeginRendering(cmd, &uiRenderingInfo);

            if (frame != 0) {
                m_uiObjects.clear();
                auto uiView = registry.view<UiComponent>();
                for (auto entity : uiView) {
                    if(uiView.get<UiComponent>(entity).m_vexUI->isInitialized())
                        m_uiObjects.emplace_back(uiView.get<UiComponent>(entity));
                }
                std::sort(m_uiObjects.begin(), m_uiObjects.end(), [](const UiComponent &f, const UiComponent &s) { return f.m_vexUI->getZIndex() < s.m_vexUI->getZIndex(); });

                for(const auto& uiObject : m_uiObjects) {
                    if(uiObject.visible){
                        uiObject.m_vexUI->render(cmd, m_p_uiPipeline->get(), m_p_uiPipeline->layout(), data.frameIndex);
                    }
                }
            }

            #if DEBUG
                if(debugLines && !debugLines->empty()) {
                    renderDebug(cmd, data.frameIndex, *debugLines);
                }
            #endif

            vkCmdEndRendering(cmd);

            transitionImageLayout(cmd, m_r_context.lowResColorImage,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         VK_ACCESS_SHADER_READ_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            transitionImageLayout(cmd, m_r_context.uiImage,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         VK_ACCESS_SHADER_READ_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        VkDescriptorSet Renderer::getImGuiTextureID(ImGUIWrapper& ui) {
            VulkanImGUIWrapper& vkUI = static_cast<VulkanImGUIWrapper&>(ui);

            auto& currentGarbage = m_garbageDescriptors[m_r_context.currentFrame];
            if (!currentGarbage.empty()) {
                for (VkDescriptorSet ds : currentGarbage) {
                    if(ds != VK_NULL_HANDLE){
                        vkUI.removeTexture(ds);
                    }
                }
                currentGarbage.clear();
            }

            if (m_cachedImGuiDescriptor == VK_NULL_HANDLE) {
                if (m_r_context.lowResColorView != VK_NULL_HANDLE) {
                    m_cachedImGuiDescriptor = vkUI.addTexture(
                        m_screenSampler,
                        m_r_context.gameViewView,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    );
                }
            }
            return m_cachedImGuiDescriptor;
        }

        void Renderer::composeFrame(SceneRenderData& data, ImGUIWrapper& ui, bool isEditorMode) {
            VkCommandBuffer cmd = data.commandBuffer;

            transitionImageLayout(cmd, m_r_context.compositeImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkRenderingAttachmentInfo preCompAtt{};
            preCompAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            preCompAtt.imageView = m_r_context.compositeView;
            preCompAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            preCompAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            preCompAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            preCompAtt.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            VkRenderingInfo preCompInfo{};
            preCompInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            preCompInfo.renderArea.offset = {0, 0};
            preCompInfo.renderArea.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y};
            preCompInfo.layerCount = 1;
            preCompInfo.colorAttachmentCount = 1;
            preCompInfo.pColorAttachments = &preCompAtt;

            vkCmdBeginRendering(cmd, &preCompInfo);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_compositePipeline->get());
            VkViewport compViewport{};
            compViewport.x = 0.0f;
            compViewport.y = 0.0f;
            compViewport.width = static_cast<float>(m_r_context.currentRenderResolution.x);
            compViewport.height = static_cast<float>(m_r_context.currentRenderResolution.y);
            compViewport.minDepth = 0.0f;
            compViewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &compViewport);
            VkRect2D compScissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
            vkCmdSetScissor(cmd, 0, 1, &compScissor);
            VkDescriptorSet compSceneSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
            uint32_t compDynamicOffset = 0;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_compositePipeline->layout(), 0, 1, &compSceneSet, 1, &compDynamicOffset);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_compositePipeline->layout(), 1, 1, &m_screenDescriptorSet, 0, nullptr);
            vkCmdDraw(cmd, 3, 1, 0, 0);
            vkCmdEndRendering(cmd);

            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = m_r_context.compositeImage;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                int32_t mipWidth = m_r_context.currentRenderResolution.x;
                int32_t mipHeight = m_r_context.currentRenderResolution.y;

                for (uint32_t i = 1; i < 3; i++) {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = (i == 1) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = (i == 1) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(cmd,
                        (i == 1) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &barrier);

                    barrier.subresourceRange.baseMipLevel = i;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    vkCmdPipelineBarrier(cmd,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;

                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(cmd,
                        m_r_context.compositeImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        m_r_context.compositeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit, VK_FILTER_LINEAR);

                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(cmd,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                barrier.subresourceRange.baseMipLevel = 2;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr, 0, nullptr, 1, &barrier);
            }


            if (isEditorMode) [[unlikely]] {
                transitionImageLayout(cmd, m_r_context.gameViewImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

                VkRenderingAttachmentInfo compAtt{};
                compAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                compAtt.imageView = m_r_context.gameViewView;
                compAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                compAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                compAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                compAtt.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                VkRenderingInfo compInfo{};
                compInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                compInfo.renderArea.offset = {0, 0};
                compInfo.renderArea.extent = {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y};
                compInfo.layerCount = 1;
                compInfo.colorAttachmentCount = 1;
                compInfo.pColorAttachments = &compAtt;

                vkCmdBeginRendering(cmd, &compInfo);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->get());
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(m_r_context.currentRenderResolution.x);
                viewport.height = static_cast<float>(m_r_context.currentRenderResolution.y);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                VkRect2D scissor{{0, 0}, {m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y}};
                vkCmdSetScissor(cmd, 0, 1, &scissor);
                VkDescriptorSet sceneSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->layout(), 0, 1, &sceneSet, 1, &dynamicOffset);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->layout(), 1, 1, &m_crtDescriptorSet, 0, nullptr);
                vkCmdDraw(cmd, 3, 1, 0, 0);
                vkCmdEndRendering(cmd);

                transitionImageLayout(cmd, m_r_context.gameViewImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }

            transitionImageLayout(cmd, m_r_context.swapchainImages[data.imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkRenderingAttachmentInfo colorAttachment{};
            colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachment.imageView = m_r_context.swapchainImageViews[data.imageIndex];
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

            VkRenderingInfo renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderingInfo.renderArea.offset = {0, 0};
            renderingInfo.renderArea.extent = m_r_context.swapchainExtent;
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;

            vkCmdBeginRendering(cmd, &renderingInfo);

            if (isEditorMode) [[unlikely]] {
                VulkanImGUIWrapper& vkUI = static_cast<VulkanImGUIWrapper&>(ui);
                if (m_cachedImGuiDescriptor == VK_NULL_HANDLE) getImGuiTextureID(ui);
                data.imguiTextureID = m_cachedImGuiDescriptor;
                vkUI.draw(cmd);
            } else [[likely]] {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->get());

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(m_r_context.swapchainExtent.width);
                viewport.height = static_cast<float>(m_r_context.swapchainExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = m_r_context.swapchainExtent;
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                VkDescriptorSet sceneSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset = 0;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->layout(), 0, 1, &sceneSet, 1, &dynamicOffset);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->layout(), 1, 1, &m_crtDescriptorSet, 0, nullptr);
                vkCmdDraw(cmd, 3, 1, 0, 0);
            }

            vkCmdEndRendering(cmd);
            transitionImageLayout(cmd, m_r_context.swapchainImages[data.imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }

        void Renderer::endFrame(SceneRenderData& data) {
            //std::cout << "currentRes x:" << m_r_context.currentRenderResolution.x << ", y:" << m_r_context.currentRenderResolution.y << std::endl;
            //std::cout << "swapchainExtent x:" << m_r_context.swapchainExtent.width << ", y:" << m_r_context.swapchainExtent.height << std::endl;
            //std::cout << "IsValid: " << data.isSwapchainValid << std::endl;
            if (!data.isSwapchainValid) return;

            vkEndCommandBuffer(data.commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {m_r_context.imageAvailableSemaphores[data.frameIndex]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &data.commandBuffer;

            VkSemaphore signalSemaphores[] = {m_r_context.renderFinishedSemaphores[data.imageIndex]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            try {
                if (vkQueueSubmit(m_r_context.graphicsQueue, 1, &submitInfo, m_r_context.inFlightFences[m_r_context.currentFrame]) != VK_SUCCESS) {
                    throw_error("Failed to submit draw command buffer!");
                }

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapchains[] = {m_r_context.swapchain};
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapchains;
                presentInfo.pImageIndices = &m_r_context.currentImageIndex;

                VkResult result = vkQueuePresentKHR(m_r_context.presentQueue, &presentInfo);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                    m_p_swapchainManager->recreateSwapchain();
                } else if (result != VK_SUCCESS) {
                    throw_error("Failed to present swap chain image!");
                }
            } catch (const std::exception& e) {
                log(LogLevel::ERROR, "Queue Submit/Present failed");
                handle_critical_exception(e);
            }

            m_r_context.currentFrame = (m_r_context.currentFrame + 1) % m_r_context.MAX_FRAMES_IN_FLIGHT;
        }

        void Renderer::updateScreenDescriptor(VkImageView view) {
            VkDescriptorImageInfo opaqueInfo{};
            opaqueInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            opaqueInfo.imageView = view;
            opaqueInfo.sampler = m_screenSampler;

            VkDescriptorImageInfo accumInfo{};
            accumInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            accumInfo.imageView = m_r_context.accumView;
            accumInfo.sampler = m_screenSampler;

            VkDescriptorImageInfo revealInfo{};
            revealInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            revealInfo.imageView = m_r_context.revealView;
            revealInfo.sampler = m_screenSampler;

            VkDescriptorImageInfo uiInfo{};
            uiInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            uiInfo.imageView = m_r_context.uiView;
            uiInfo.sampler = m_screenSampler;

            VkDescriptorImageInfo lutInfo{};
            lutInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            lutInfo.imageView = m_r_context.colorLutView;
            lutInfo.sampler = m_linearSampler;

            std::array<VkWriteDescriptorSet, 5> writes{};

            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = m_screenDescriptorSet;
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &opaqueInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = m_screenDescriptorSet;
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo = &accumInfo;

            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = m_screenDescriptorSet;
            writes[2].dstBinding = 2;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[2].descriptorCount = 1;
            writes[2].pImageInfo = &revealInfo;

            writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[3].dstSet = m_screenDescriptorSet;
            writes[3].dstBinding = 3;
            writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[3].descriptorCount = 1;
            writes[3].pImageInfo = &uiInfo;

            writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[4].dstSet = m_screenDescriptorSet;
            writes[4].dstBinding = 4;
            writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[4].descriptorCount = 1;
            writes[4].pImageInfo = &lutInfo;

            vkUpdateDescriptorSets(m_r_context.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

            VkDescriptorImageInfo compInfo{};
            compInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (m_r_context.compositeView != VK_NULL_HANDLE) {
                compInfo.imageView = m_r_context.compositeView;
            } else {
                compInfo.imageView = view;
            }
            compInfo.sampler = m_screenSampler;

            VkDescriptorImageInfo compLinearInfo = compInfo;
            compLinearInfo.sampler = m_linearSampler;

            VkWriteDescriptorSet compWrite[4] = {};
            for(int i = 0; i < 4; i++) {
                compWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                compWrite[i].dstSet = m_crtDescriptorSet;
                compWrite[i].dstBinding = i;
                compWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                compWrite[i].descriptorCount = 1;
                compWrite[i].pImageInfo = (i == 1) ? &compLinearInfo : &compInfo;
            }

            vkUpdateDescriptorSets(m_r_context.device, 4, compWrite, 0, nullptr);

        }

    void Renderer::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                         VkImageLayout oldLayout, VkImageLayout newLayout,
                                         VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if (m_r_context.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                    m_r_context.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT) {
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            } else {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            cmd,
            srcStage,
            dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void Renderer::issueMultiDrawIndexed(VkCommandBuffer cmd, const std::vector<VkMultiDrawIndexedInfoEXT>& commands) {
        if (commands.empty()) return;

        if (m_r_context.supportsMultiDraw && m_r_context.maxMultiDrawCount > 0) [[likely]] {
            const uint32_t limit = m_r_context.maxMultiDrawCount;
            size_t remaining = commands.size();
            size_t offset = 0;

            while (remaining > 0) {
                uint32_t count = static_cast<uint32_t>(std::min(static_cast<size_t>(limit), remaining));

                vkCmdDrawMultiIndexedEXT(
                    cmd,
                    count,
                    commands.data() + offset,
                    1,
                    0,
                    static_cast<uint32_t>(sizeof(VkMultiDrawIndexedInfoEXT)),
                    nullptr
                );

                remaining -= count;
                offset += count;
            }
            return;
        }

        if(basicDiag) [[unlikely]] {
            log(LogLevel::WARNING, "MultiDraw fallback active. Count: %zu", commands.size());
            basicDiag = false;
        }

        for (const auto& draw : commands) {
            vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
        }
    }
}
