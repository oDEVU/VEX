#include "Renderer.hpp"
#include "components/backends/vulkan/Pipeline.hpp"
#include "components/backends/vulkan/uniforms.hpp"
#include "entt/entity/fwd.hpp"
#include <cstdint>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <components/GameComponents/BasicComponents.hpp>
#include "frustum.hpp"
#include "limits.hpp"

#if defined(max)
    #undef max
#endif

namespace vex {
    glm::vec3 extractCameraPosition(const glm::mat4& view) {
        glm::mat4 invView = glm::inverse(view);
        return glm::vec3(invView[3]);
    }

    Renderer::Renderer(VulkanContext& context,
                           std::unique_ptr<VulkanResources>& resources,
                           std::unique_ptr<VulkanPipeline>& pipeline,
                           std::unique_ptr<VulkanPipeline>& transPipeline,
                           std::unique_ptr<VulkanPipeline>& uiPipeline,
                           std::unique_ptr<VulkanPipeline>& fullscreenPipeline,
                           std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                           std::unique_ptr<MeshManager>& meshManager)
            : m_r_context(context),
              m_p_resources(resources),
              m_p_pipeline(pipeline),
              m_p_transPipeline(transPipeline),
              m_p_uiPipeline(uiPipeline),
              m_p_fullscreenPipeline(fullscreenPipeline),
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

                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

                VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = 1;
                poolInfo.pPoolSizes = &poolSize;
                poolInfo.maxSets = 1;

                vkCreateDescriptorPool(m_r_context.device, &poolInfo, nullptr, &m_localPool);

                allocInfo.descriptorPool = m_localPool;
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &m_r_context.textureDescriptorSetLayout;

                vkAllocateDescriptorSets(m_r_context.device, &allocInfo, &m_screenDescriptorSet);

        log("Renderer initialized successfully");
    }

    Renderer::~Renderer() {
        if (m_screenSampler) vkDestroySampler(m_r_context.device, m_screenSampler, nullptr);
        if (m_localPool) vkDestroyDescriptorPool(m_r_context.device, m_localPool, nullptr);
        log("Renderer destroyed");
    }

    bool Renderer::beginFrame(glm::uvec2 renderResolution, SceneRenderData& outData) {
        try {
            if (renderResolution != m_r_context.currentRenderResolution) {
                log(LogLevel::WARNING, "Renderer doesnt match swapchain %d x %d =/= %d x %d", m_r_context.currentRenderResolution.x, m_r_context.currentRenderResolution.y, renderResolution.x, renderResolution.y);
                m_r_context.currentRenderResolution = renderResolution;
                m_p_pipeline->updateViewport(renderResolution);
                m_p_swapchainManager->recreateSwapchain();
                m_lastUsedView = VK_NULL_HANDLE;
                m_cachedImGuiDescriptor = VK_NULL_HANDLE;
                updateScreenDescriptor(m_r_context.lowResColorView);
            }

            vkWaitForFences(m_r_context.device, 1, &m_r_context.inFlightFences[m_r_context.currentFrame], VK_TRUE, UINT64_MAX);

            VkResult result = vkAcquireNextImageKHR(
                m_r_context.device,
                m_r_context.swapchain,
                UINT64_MAX,
                m_r_context.imageAvailableSemaphores[m_r_context.currentFrame],
                VK_NULL_HANDLE,
                &m_r_context.currentImageIndex
            );

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
            handle_exception(e);
            return false;
        }
    }

        void Renderer::renderScene(SceneRenderData& data, const entt::entity cameraEntity, entt::registry& registry, int frame, bool isEditorMode) {
            VkCommandBuffer cmd = data.commandBuffer;

            auto now = std::chrono::high_resolution_clock::now();
            currentTime = std::chrono::duration<float>(now - startTime).count();

            if (m_lastUsedView != m_r_context.lowResColorView) {
                updateScreenDescriptor(m_r_context.lowResColorView);
                m_cachedImGuiDescriptor = VK_NULL_HANDLE;
                m_lastUsedView = m_r_context.lowResColorView;
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
            colorAttachment.clearValue.color = {{m_r_context.m_enviroment.clearColor.x, m_r_context.m_enviroment.clearColor.y, m_r_context.m_enviroment.clearColor.z, 1.0f}};

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
                handle_exception(e);
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

            view = glm::lookAt(transform.getWorldPosition(), transform.getWorldPosition() + transform.getForwardVector(), transform.getUpVector());
            proj = glm::perspective(glm::radians(camera.fov), (float)m_r_context.currentRenderResolution.x / (float)m_r_context.currentRenderResolution.y, camera.nearPlane, camera.farPlane);
            proj[1][1] *= -1;

            //log("Updating scene UBO...");
            m_sceneUBO.view = view;
            m_sceneUBO.proj = proj;

            m_sceneUBO.snapResolution = 1.f;
            m_sceneUBO.jitterIntensity = 0.5f;

            m_sceneUBO.enablePS1Effects = 0;

            if(m_r_context.m_enviroment.vertexSnapping){
                m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_SNAPPING;
            }

            if(m_r_context.m_enviroment.passiveVertexJitter){
                m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_JITTER;
            }

            if(m_r_context.m_enviroment.affineWarping){
                m_sceneUBO.enablePS1Effects |= PS1Effects::AFFINE_WARPING;
            }

            if(m_r_context.m_enviroment.screenQuantization){
                m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_QUANTIZATION;
            }

            if(m_r_context.m_enviroment.ntfsArtifacts){
                m_sceneUBO.enablePS1Effects |= PS1Effects::NTSC_ARTIFACTS;
            }

            if(m_r_context.m_enviroment.gourardShading){
                m_sceneUBO.enablePS1Effects |= PS1Effects::GOURAUD_SHADING;
            }

            if(m_r_context.m_enviroment.textureQuantization){
                m_sceneUBO.enablePS1Effects |= PS1Effects::TEXTURE_QUANTIZATION;
            }

            if(m_r_context.m_enviroment.screenDither){
                m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_DITHER;
            }

            m_sceneUBO.renderResolution = m_r_context.currentRenderResolution;
            m_sceneUBO.windowResolution = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
            m_sceneUBO.time = currentTime;
            m_sceneUBO.frame = frame;
            m_sceneUBO.upscaleRatio = m_r_context.swapchainExtent.height / static_cast<float>(m_r_context.currentRenderResolution.y);

            m_sceneUBO.ambientLight = glm::vec4(m_r_context.m_enviroment.ambientLight,1.0f);
            m_sceneUBO.ambientLightStrength = m_r_context.m_enviroment.ambientLightStrength;
            m_sceneUBO.sunLight = glm::vec4(m_r_context.m_enviroment.sunLight,1.0f);
            m_sceneUBO.sunDirection = glm::vec4(m_r_context.m_enviroment.sunDirection,1.0f);

            m_p_resources->updateSceneUBO(m_sceneUBO);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());

            glm::vec3 cameraPos = extractCameraPosition(view);
            Frustum camFrustum;
            camFrustum.update(proj * view);
            m_transparentTriangles.clear();
            trnasMatrixes.clear();
            uint32_t modelIndex = 0;

            auto modelView = registry.view<TransformComponent, MeshComponent>();
            for (auto entity : modelView) {
                auto& transform = modelView.get<TransformComponent>(entity);
                auto& mesh = modelView.get<MeshComponent>(entity);
                glm::mat4 modelMatrix = transform.matrix();

                if(!transform.isReady()){
                    transform.setRegistry(registry);
                }

                if(transform.transformedLately() || mesh.getIsFresh() || transform.isPhysicsAffected() || isEditorMode || mesh.worldRadius <= 0.0f){
                    mesh.worldCenter = (modelMatrix * glm::vec4(mesh.localCenter, 1.0f));
                    mesh.worldRadius = mesh.localRadius * glm::max(transform.getWorldScale().x, transform.getWorldScale().y, transform.getWorldScale().z);

                    #if DEBUG
                    if(isEditorMode && frame > 0){
                        transform.setLocalRotation(transform.rotation);
                    }else if(isEditorMode){
                        transform.rotation = transform.getLocalRotation();
                    }
                    transform.rotation = glm::mod(transform.rotation, glm::vec3(360.0f));
                    #endif
                }

                if (!camFrustum.testSphere(mesh.worldCenter, mesh.worldRadius)) {
                    continue;
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
                    auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);
                    if (vulkanMesh) {
                        vulkanMesh->draw(cmd, m_p_pipeline->layout(), *m_p_resources, data.frameIndex, modelIndex, modelMatrix, mesh.color);
                    }
                } else if (mesh.renderType == RenderType::TRANSPARENT) {
                     auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);

                     if (vulkanMesh) {
                         vulkanMesh->extractTransparentTriangles(
                             modelMatrix,
                             cameraPos,
                             modelIndex,
                             m_r_context.currentFrame,
                             m_transparentTriangles
                         );
                         trnasMatrixes[modelIndex] = modelMatrix;
                     }
                }
                modelIndex++;
                if(mesh.getIsFresh()) mesh.setRendered();
            }

            if (!m_transparentTriangles.empty()) {
                std::sort(m_transparentTriangles.begin(), m_transparentTriangles.end(),
                          [](const auto& a, const auto& b) {
                              if (a.distanceToCamera != b.distanceToCamera) {
                                  return a.distanceToCamera > b.distanceToCamera;
                              }

                              if (a.modelIndex != b.modelIndex) {
                                  return a.modelIndex < b.modelIndex;
                              }

                              return a.submeshIndex < b.submeshIndex;
                          });

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_transPipeline->get());

                VulkanMesh* batchMesh = nullptr;
                uint32_t batchSubmeshIndex = UINT32_MAX;
                uint32_t batchModelIndex = UINT32_MAX;
                const auto& [key, value] = *trnasMatrixes.begin();
                glm::mat4 batchModelMatrix = value;

                for (const auto& tri : m_transparentTriangles) {
                    bool stateChange = (tri.mesh != batchMesh ||
                                        tri.submeshIndex != batchSubmeshIndex ||
                                        tri.modelIndex != batchModelIndex);

                    if (stateChange && !m_multiDrawInfos.empty()) {
                        batchMesh->bindAndDrawBatched(
                            cmd,
                            m_p_transPipeline->layout(),
                            *m_p_resources,
                            m_r_context.currentFrame,
                            batchModelIndex,
                            batchSubmeshIndex,
                            batchModelMatrix,
                            true,//tri.modelIndex != batchModelIndex, // <- Doesnt work. Why? Needs to be fixed.
                            tri.submeshIndex != batchSubmeshIndex
                        );

                        issueMultiDrawIndexed(cmd, m_multiDrawInfos);
                        m_multiDrawInfos.clear();
                    }

                    if (stateChange) {
                        batchMesh = tri.mesh;
                        batchSubmeshIndex = tri.submeshIndex;
                        batchModelIndex = tri.modelIndex;
                        batchModelMatrix = trnasMatrixes[tri.modelIndex];
                    }

                    bool canMerge = !m_multiDrawInfos.empty() && !stateChange;
                    if (canMerge) {
                        auto& lastDraw = m_multiDrawInfos.back();
                        if (tri.firstIndex == (lastDraw.firstIndex + lastDraw.indexCount)) {
                            lastDraw.indexCount += 3;
                            continue;
                        }
                    }

                    VkMultiDrawIndexedInfoEXT drawInfo{};
                    drawInfo.firstIndex = tri.firstIndex;
                    drawInfo.indexCount = 3;
                    drawInfo.vertexOffset = 0;
                    m_multiDrawInfos.push_back(drawInfo);
                }

                if (!m_multiDrawInfos.empty() && batchMesh != nullptr) {
                    batchMesh->bindAndDrawBatched(
                        cmd,
                        m_p_transPipeline->layout(),
                        *m_p_resources,
                        m_r_context.currentFrame,
                        batchModelIndex,
                        batchSubmeshIndex,
                        batchModelMatrix,
                        true,
                        true
                    );
                    issueMultiDrawIndexed(cmd, m_multiDrawInfos);
                }

                m_multiDrawInfos.clear();
            }


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

            vkCmdEndRendering(cmd);

            transitionImageLayout(cmd, m_r_context.lowResColorImage,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         VK_ACCESS_SHADER_READ_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        VkDescriptorSet Renderer::getImGuiTextureID(ImGUIWrapper& ui) {
            if (m_cachedImGuiDescriptor == VK_NULL_HANDLE) {
                VulkanImGUIWrapper& vkUI = static_cast<VulkanImGUIWrapper&>(ui);
                if (m_r_context.lowResColorView != VK_NULL_HANDLE) {
                    m_cachedImGuiDescriptor = vkUI.addTexture(
                        m_screenSampler,
                        m_r_context.lowResColorView,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    );
                }
            }
            return m_cachedImGuiDescriptor;
        }

        void Renderer::composeFrame(SceneRenderData& data, ImGUIWrapper& ui, bool isEditorMode) {
            VkCommandBuffer cmd = data.commandBuffer;

            transitionImageLayout(cmd, m_r_context.swapchainImages[data.imageIndex],
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 0,
                                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

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


            try {
                vkCmdBeginRendering(cmd, &renderingInfo);
            } catch (const std::exception& e) {
                handle_exception(e);
                return;
            }

            VkViewport viewport{};
            viewport.width = (float)m_r_context.swapchainExtent.width;
            viewport.height = (float)m_r_context.swapchainExtent.height;
            viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = m_r_context.swapchainExtent;
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            if (isEditorMode) {
                    VulkanImGUIWrapper& vkUI = static_cast<VulkanImGUIWrapper&>(ui);

                    if (m_cachedImGuiDescriptor == VK_NULL_HANDLE) {
                        getImGuiTextureID(ui);
                    }
                    data.imguiTextureID = m_cachedImGuiDescriptor;

                    vkUI.draw(cmd);
            } else {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->get());

                VkDescriptorSet sceneSet = m_p_resources->getUBODescriptorSet(data.frameIndex);
                uint32_t dynamicOffset = 0;

                vkCmdBindDescriptorSets(
                        cmd,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        m_p_fullscreenPipeline->layout(),
                        0,
                        1,
                        &sceneSet,//&m_r_context.descriptorSets[data.frameIndex],
                        1,
                        &dynamicOffset
                    );

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fullscreenPipeline->layout(),
                                        1, 1, &m_screenDescriptorSet, 0, nullptr);

                vkCmdDraw(cmd, 3, 1, 0, 0);
            }

            vkCmdEndRendering(cmd);

            transitionImageLayout(cmd, m_r_context.swapchainImages[data.imageIndex],
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                 0,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
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

            VkSemaphore signalSemaphores[] = {m_r_context.renderFinishedSemaphores[data.frameIndex]};
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
                handle_exception(e);
            }

            m_r_context.currentFrame = (m_r_context.currentFrame + 1) % m_r_context.MAX_FRAMES_IN_FLIGHT;
        }

        void Renderer::updateScreenDescriptor(VkImageView view) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = view;
            imageInfo.sampler = m_screenSampler;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_screenDescriptorSet;
            write.dstBinding = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_r_context.device, 1, &write, 0, nullptr);
        }

    void Renderer::renderFrame(const entt::entity cameraEntity, glm::uvec2 renderResolution, entt::registry& registry, ImGUIWrapper& m_ui, uint64_t frame) {
        //log("Begining rendering...");
        if (renderResolution != m_r_context.currentRenderResolution) {
            m_r_context.currentRenderResolution = renderResolution;
            m_p_pipeline->updateViewport(renderResolution);
        }
        vkWaitForFences(
            m_r_context.device,
            1,
            &m_r_context.inFlightFences[m_r_context.currentFrame],
            VK_TRUE,
            UINT64_MAX
        );

        VkResult result = vkAcquireNextImageKHR(
            m_r_context.device,
            m_r_context.swapchain,
            UINT64_MAX,
            m_r_context.imageAvailableSemaphores[m_r_context.currentFrame],
            VK_NULL_HANDLE,
            &m_r_context.currentImageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_p_swapchainManager->recreateSwapchain();
            return;
        }else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw_error("Failed to acquire swap chain image!");
        }

        vkResetFences(m_r_context.device, 1, &m_r_context.inFlightFences[m_r_context.currentFrame]);

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);

        auto& transform = registry.get<TransformComponent>(cameraEntity);
        auto& camera = registry.get<CameraComponent>(cameraEntity);

        if(!transform.isReady()){
            transform.setRegistry(registry);
        }

        view = glm::lookAt(
            transform.getWorldPosition(),
            transform.getWorldPosition() + transform.getForwardVector(),
            transform.getUpVector()
        );
        proj = glm::perspective(
            glm::radians(camera.fov),
            renderResolution.x / static_cast<float>(renderResolution.y),
            camera.nearPlane,
            camera.farPlane
        );
        proj[1][1] *= -1;

        //log("Updating scene UBO...");
        m_sceneUBO.view = view;
        m_sceneUBO.proj = proj;

        m_sceneUBO.snapResolution = 1.f;
        m_sceneUBO.jitterIntensity = 0.5f;

        m_sceneUBO.enablePS1Effects = 0;

        if(m_r_context.m_enviroment.vertexSnapping){
            m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_SNAPPING;
        }

        if(m_r_context.m_enviroment.passiveVertexJitter){
            m_sceneUBO.enablePS1Effects |= PS1Effects::VERTEX_JITTER;
        }

        if(m_r_context.m_enviroment.affineWarping){
            m_sceneUBO.enablePS1Effects |= PS1Effects::AFFINE_WARPING;
        }

        if(m_r_context.m_enviroment.screenQuantization){
            m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_QUANTIZATION;
        }

        if(m_r_context.m_enviroment.ntfsArtifacts){
            m_sceneUBO.enablePS1Effects |= PS1Effects::NTSC_ARTIFACTS;
        }

        if(m_r_context.m_enviroment.gourardShading){
            m_sceneUBO.enablePS1Effects |= PS1Effects::GOURAUD_SHADING;
        }

        if(m_r_context.m_enviroment.textureQuantization){
            m_sceneUBO.enablePS1Effects |= PS1Effects::TEXTURE_QUANTIZATION;
        }

        if(m_r_context.m_enviroment.screenDither){
            m_sceneUBO.enablePS1Effects |= PS1Effects::SCREEN_DITHER;
        }

        m_sceneUBO.renderResolution = m_r_context.currentRenderResolution;
        m_sceneUBO.windowResolution = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
        m_sceneUBO.time = currentTime;
        m_sceneUBO.upscaleRatio = m_r_context.swapchainExtent.height / static_cast<float>(m_r_context.currentRenderResolution.y);

        m_sceneUBO.ambientLight = glm::vec4(m_r_context.m_enviroment.ambientLight,1.0f);
        m_sceneUBO.ambientLightStrength = m_r_context.m_enviroment.ambientLightStrength;
        m_sceneUBO.sunLight = glm::vec4(m_r_context.m_enviroment.sunLight,1.0f);
        m_sceneUBO.sunDirection = glm::vec4(m_r_context.m_enviroment.sunDirection,1.0f);

        m_p_resources->updateSceneUBO(m_sceneUBO);

        //log("Updating texture Descriptor.");
        //VkImageView textureView = m_p_resources->getTextureView("default");
        //m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, textureView, 0);

        //log("Begining comand buffer...");
        VkCommandBuffer commandBuffer = m_r_context.commandBuffers[m_r_context.currentFrame];
        //vkResetCommandBuffer(commandBuffer, 0);
        vkResetCommandPool(
            m_r_context.device,
            m_r_context.commandPools[m_r_context.currentFrame],
            0
        );

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        //log("Translating high res image...");
        transitionImageLayout(commandBuffer,
            m_r_context.swapchainImages[m_r_context.currentImageIndex],
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_MEMORY_READ_BIT, // src access appropriate for present
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // conservative
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        //log("Translating low res image...");
        transitionImageLayout(commandBuffer,
                             m_r_context.lowResColorImage,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             0,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = {renderResolution.x, renderResolution.y};
        renderingInfo.layerCount = 1;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_r_context.lowResColorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {{m_r_context.m_enviroment.clearColor.x, m_r_context.m_enviroment.clearColor.y, m_r_context.m_enviroment.clearColor.z, 1.0f}};

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = m_r_context.depthImageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};

        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        //log("Beggining rendering...");
        try{
            vkCmdBeginRendering(commandBuffer, &renderingInfo);
        } catch (const std::exception& e) {
            handle_exception(e);
        }
        //log("vkCmdBeginRendering ran");

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_r_context.swapchainExtent.width);
        viewport.height = static_cast<float>(m_r_context.swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        //log("setting viewport");
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        //log("vkCmdSetViewport ran");

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
        //log("setting scissor");
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        //log("vkCmdSetScissor ran");

        //VkImageView defaultTexture = m_p_resources->getTextureView("default");
        //m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, defaultTexture, 0);


        //log("Binding pipeline...");
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());

        auto now = std::chrono::high_resolution_clock::now();
        currentTime = std::chrono::duration<float>(now - startTime).count();

                //std::vector<TransparentTriangle> m_transparentTriangles;

                m_transparentTriangles.clear();
                //if(approxTriangles!=0){
                //    m_transparentTriangles.reserve(approxTriangles);
                //}

                //log("Getting renderable entities...");
                auto modelView = registry.view<TransformComponent, MeshComponent>();
                uint32_t modelIndex = 0;
                //std::vector<std::pair<entt::entity, float>> transparentEntities;


                //log("Extracting camera pos...");
                glm::vec3 cameraPos = extractCameraPosition(view);
                Frustum camFrustum;
                camFrustum.update(proj * view);

                trnasMatrixes.clear();

                //log("Rendering non transparent meshes");
                for (auto entity : modelView) {
                    auto& transform = modelView.get<TransformComponent>(entity);
                    auto& mesh = modelView.get<MeshComponent>(entity);

                    if(!transform.isReady()){
                        transform.setRegistry(registry);
                    }

                    glm::mat4 modelMatrix = transform.matrix();
                    //m_p_resources->updateModelUBO(m_r_context.currentFrame, modelIndex, ModelUBO{modelMatrix});

                    if(transform.transformedLately() || mesh.getIsFresh() || transform.isPhysicsAffected() || mesh.worldRadius <= 0.0f){
                        mesh.worldCenter = (modelMatrix * glm::vec4(mesh.localCenter, 1.0f));
                        mesh.worldRadius = mesh.localRadius * glm::max(transform.getWorldScale().x, transform.getWorldScale().y, transform.getWorldScale().z);
                    }

                    if (!camFrustum.testSphere(mesh.worldCenter, mesh.worldRadius)) {
                        //log("Mesh %s is outside of camera frustum", mesh.meshData.meshPath.c_str());
                        continue;
                    }

                    //m_lights.clear();
                    auto lightView = registry.view<TransformComponent, LightComponent>();

                    SceneLightsUBO lightUBO;
                    lightUBO.lightCount = 0;

                    for(auto lightEntity : lightView){

                        if (lightUBO.lightCount >= MAX_DYNAMIC_LIGHTS) {
                            lightUBO.lightCount = MAX_DYNAMIC_LIGHTS;
                            log("Limit reached of per object dynamic lights, rest of the light wont affect this mesh.");
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
                        auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(modelView.get<MeshComponent>(entity));

                        if (vulkanMesh) {
                            vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_r_context.currentFrame, modelIndex, modelMatrix, modelView.get<MeshComponent>(entity).color);
                        }//modelIndex++;
                    } else if (mesh.renderType == RenderType::TRANSPARENT) {
                        //float distance = glm::length(transform.getWorldPosition(registry) - cameraPos);
                        //transparentEntities.emplace_back(entity, distance);
                        //
                        //float distToCam = glm::distance(transform.getWorldPosition(), cameraPos);
                        //if (distToCam < camera.farPlane + mesh.worldRadius) {
                            auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(mesh);

                            if (vulkanMesh) {
                                vulkanMesh->extractTransparentTriangles(
                                    modelMatrix,
                                    cameraPos,
                                    modelIndex,
                                    m_r_context.currentFrame,
                                    m_transparentTriangles
                                );

                                trnasMatrixes[modelIndex] = modelMatrix;
                            }
                    }
                    modelIndex++;

                    if(mesh.getIsFresh()){
                        mesh.setRendered();
                    }
                }

                std::sort(m_transparentTriangles.begin(), m_transparentTriangles.end(),
                          [](const auto& a, const auto& b) {
                              if (a.distanceToCamera != b.distanceToCamera) {
                                  return a.distanceToCamera > b.distanceToCamera;
                              }

                              if (a.modelIndex != b.modelIndex) {
                                  return a.modelIndex < b.modelIndex;
                              }

                              return a.submeshIndex < b.submeshIndex;
                          });

                //approxTriangles = m_transparentTriangles.size() + 1024;

                //log("Drawind transparent meshes");
                /*for (const auto& [entity, distance] : transparentEntities) {
                    auto& transform = modelView.get<TransformComponent>(entity);
                    auto& mesh = modelView.get<MeshComponent>(entity);
                    m_p_resources->updateModelUBO(m_r_context.currentFrame, modelIndex, ModelUBO{transform.matrix(registry)});
                    auto& vulkanMesh = m_p_meshManager->getVulkanMeshByMesh(modelView.get<MeshComponent>(entity).meshData.meshPath);
                    vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_r_context.currentFrame, modelIndex, currentTime, m_r_context.currentRenderResolution);
                    modelIndex++;
                    }*/

                    /*for (const auto& tri : m_transparentTriangles) {
                        tri.mesh->drawTriangle(
                            commandBuffer,
                            m_p_pipeline->layout(),
                            *m_p_resources,
                            tri.frameIndex,
                            tri.modelIndex,
                            tri.submeshIndex,
                            tri.firstIndex,
                            currentTime,
                            m_r_context.currentRenderResolution
                        );
                        }*/

                        //VulkanMesh* currentMesh = nullptr;
                        //uint32_t currentSubmeshIndex = UINT32_MAX;
                        //uint32_t currentModelIndex = UINT32_MAX;
                        //float lastTime = 0.0f;

                        /*for (const auto& tri : m_transparentTriangles) {
                            if (tri.mesh != currentMesh ||
                                tri.submeshIndex != currentSubmeshIndex ||
                                tri.modelIndex != currentModelIndex ||
                                currentTime != lastTime)
                            {

                                tri.mesh->bindAndDrawBatched(
                                    commandBuffer,
                                    m_p_pipeline->layout(),
                                    *m_p_resources,
                                    tri.frameIndex,
                                    tri.modelIndex,
                                    tri.submeshIndex,
                                    currentTime,
                                    m_r_context.currentRenderResolution
                                );

                                currentMesh = tri.mesh;
                                currentSubmeshIndex = tri.submeshIndex;
                                currentModelIndex = tri.modelIndex;
                                lastTime = currentTime;
                            }

                            vkCmdDrawIndexed(commandBuffer, 3, 1, tri.firstIndex, 0, 0);
                            }*/
                            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_transPipeline->get());

                            VulkanMesh* batchMesh = nullptr;
                            uint32_t batchSubmeshIndex = UINT32_MAX;
                            uint32_t batchModelIndex = UINT32_MAX;
                            const auto& [key, value] = *trnasMatrixes.begin();
                            glm::mat4 batchModelMatrix = value;

                            for (const auto& tri : m_transparentTriangles) {
                                bool stateChange = (tri.mesh != batchMesh ||
                                                    tri.submeshIndex != batchSubmeshIndex ||
                                                    tri.modelIndex != batchModelIndex);

                                if (stateChange && !m_multiDrawInfos.empty()) {
                                    batchMesh->bindAndDrawBatched(
                                        commandBuffer,
                                        m_p_transPipeline->layout(),
                                        *m_p_resources,
                                        m_r_context.currentFrame,
                                        batchModelIndex,
                                        batchSubmeshIndex,
                                        batchModelMatrix,
                                        true,//tri.modelIndex != batchModelIndex, // <- Doesnt work. Why? Needs to be fixed.
                                        tri.submeshIndex != batchSubmeshIndex
                                    );

                                    issueMultiDrawIndexed(commandBuffer, m_multiDrawInfos);
                                    m_multiDrawInfos.clear();
                                }

                                if (stateChange) {
                                    batchMesh = tri.mesh;
                                    batchSubmeshIndex = tri.submeshIndex;
                                    batchModelIndex = tri.modelIndex;
                                    batchModelMatrix = trnasMatrixes[tri.modelIndex];
                                }

                                bool canMerge = !m_multiDrawInfos.empty() && !stateChange;
                                if (canMerge) {
                                    auto& lastDraw = m_multiDrawInfos.back();
                                    if (tri.firstIndex == (lastDraw.firstIndex + lastDraw.indexCount)) {
                                        lastDraw.indexCount += 3;
                                        continue;
                                    }
                                }

                                VkMultiDrawIndexedInfoEXT drawInfo{};
                                drawInfo.firstIndex = tri.firstIndex;
                                drawInfo.indexCount = 3;
                                drawInfo.vertexOffset = 0;
                                m_multiDrawInfos.push_back(drawInfo);
                            }

                            if (!m_multiDrawInfos.empty() && batchMesh != nullptr) {
                                batchMesh->bindAndDrawBatched(
                                    commandBuffer,
                                    m_p_transPipeline->layout(),
                                    *m_p_resources,
                                    m_r_context.currentFrame,
                                    batchModelIndex,
                                    batchSubmeshIndex,
                                    batchModelMatrix,
                                    true,
                                    true
                                );
                                issueMultiDrawIndexed(commandBuffer, m_multiDrawInfos);
                            }

                            m_multiDrawInfos.clear();

        if (frame != 0) {
            //log("Rendering VEXUI and ImGUI...");
            //vui.render(commandBuffer, m_p_uiPipeline->get(), m_p_uiPipeline->layout(), m_r_context.currentFrame);
            //
            m_uiObjects.clear();
            auto uiView = registry.view<UiComponent>();
            for (auto entity : uiView) {
                if(uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                    m_uiObjects.emplace_back(uiView.get<UiComponent>(entity));
                }else{
                    log("UiComponent not initialized");
                }
            }

            std::sort(m_uiObjects.begin(), m_uiObjects.end(), [](const UiComponent &f, const UiComponent &s) { return f.m_vexUI->getZIndex() < s.m_vexUI->getZIndex(); });

            for(const auto& uiObject : m_uiObjects) {
                if(uiObject.visible){
                    uiObject.m_vexUI->render(commandBuffer, m_p_uiPipeline->get(), m_p_uiPipeline->layout(), m_r_context.currentFrame);
                }
            }

            m_ui.beginFrame();
            m_ui.executeUIFunctions();
            m_ui.endFrame();
        }


        //log("Ending rendering...");
        vkCmdEndRendering(commandBuffer);


        //log("Translating low res color image...");
        transitionImageLayout(commandBuffer, m_r_context.lowResColorImage,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);


        //log("Bliting low res image to highres target...");
        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {(int)renderResolution.x, (int)renderResolution.y, 1};

        blit.dstSubresource = blit.srcSubresource;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {(int)m_r_context.swapchainExtent.width, (int)m_r_context.swapchainExtent.height, 1};

        vkCmdBlitImage(
            commandBuffer,
            m_r_context.lowResColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_r_context.swapchainImages[m_r_context.currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_NEAREST
        );

        VkImageMemoryBarrier presentBarrier{};
        presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.image = m_r_context.swapchainImages[m_r_context.currentImageIndex];
        presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.layerCount = 1;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;

        VkImageMemoryBarrier lowResRestoreBarrier{};
        lowResRestoreBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lowResRestoreBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        lowResRestoreBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lowResRestoreBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lowResRestoreBarrier.image = m_r_context.lowResColorImage;
        lowResRestoreBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        lowResRestoreBarrier.subresourceRange.levelCount = 1;
        lowResRestoreBarrier.subresourceRange.layerCount = 1;
        lowResRestoreBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        lowResRestoreBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        //log("vkCmdPipelineBarrier.");
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            2, (VkImageMemoryBarrier[]){presentBarrier, lowResRestoreBarrier}
        );


        //log("Ending command buffer.");
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_r_context.imageAvailableSemaphores[m_r_context.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {m_r_context.renderFinishedSemaphores[m_r_context.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;


        //log("Queue submit.");
        vkQueueSubmit(
            m_r_context.graphicsQueue,
            1,
            &submitInfo,
            m_r_context.inFlightFences[m_r_context.currentFrame]
        );

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = {m_r_context.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &m_r_context.currentImageIndex;


        //log("queue present.");
        result = vkQueuePresentKHR(m_r_context.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_p_swapchainManager->recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw_error("Failed to present swap chain image!");
        }

        m_r_context.currentFrame = (m_r_context.currentFrame + 1) % m_r_context.MAX_FRAMES_IN_FLIGHT;
        //log("Frame in flight %d", m_r_context.currentFrame);
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
        //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                // Dodaj aspekt Stencil, jeśli używasz formatu z S8_UINT.
                // W SwapchainManager.cpp masz formaty z S8_UINT, więc jest to bezpieczne.
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

                if (m_r_context.supportsMultiDraw) {
                    vkCmdDrawMultiIndexedEXT(
                        cmd,
                        static_cast<uint32_t>(commands.size()),
                        commands.data(),
                        1,
                        0,
                        static_cast<uint32_t>(sizeof(VkMultiDrawIndexedInfoEXT)),
                        nullptr
                    );
                } else {
                    if(basicDiag){
                        log("MultiDraw unsuported using fallback");
                        basicDiag = false;
                    }
                    for (const auto& draw : commands) {
                        vkCmdDrawIndexed(
                            cmd,
                            draw.indexCount,
                            1,
                            draw.firstIndex,
                            draw.vertexOffset,
                            0
                        );
                    }
                }
    }
}
