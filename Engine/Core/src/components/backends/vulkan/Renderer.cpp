#include "Renderer.hpp"
#include "components/backends/vulkan/Pipeline.hpp"
#include "entt/entity/fwd.hpp"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include <components/GameComponents/BasicComponents.hpp>
#include <components/GameComponents/UiComponent.hpp>

namespace vex {
    glm::vec3 extractCameraPosition(const glm::mat4& view) {
        glm::mat4 invView = glm::inverse(view);
        return glm::vec3(invView[3]);
    }

    Renderer::Renderer(VulkanContext& context,
                       std::shared_ptr<VulkanResources>& resources,
                       std::unique_ptr<VulkanPipeline>& pipeline,
                       std::unique_ptr<VulkanPipeline>& uiPipeline,
                       std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                       std::unique_ptr<MeshManager>& meshManager)
        : m_r_context(context),
          m_p_resources(resources),
          m_p_pipeline(pipeline),
          m_p_uiPipeline(uiPipeline),
          m_p_swapchainManager(swapchainManager),
          m_p_meshManager(meshManager) {
        startTime = std::chrono::high_resolution_clock::now();
        log("Renderer initialized successfully");
    }

    Renderer::~Renderer() {
        log("Renderer destroyed");
    }

    void Renderer::renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution, entt::registry& registry, ImGUIWrapper& m_ui, uint64_t frame) {
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

        vkDeviceWaitIdle(m_r_context.device);

        //log("Updating camera UBO...");
        m_p_resources->updateCameraUBO({view, proj});

        //log("Updating texture Descriptor.");
        VkImageView textureView = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, textureView, 0);

        //log("Begining comand buffer...");
        VkCommandBuffer commandBuffer = m_r_context.commandBuffers[m_r_context.currentImageIndex];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        //log("Translating high res image...");
        transitionImageLayout(commandBuffer,
                             m_r_context.swapchainImages[m_r_context.currentImageIndex],
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             0,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
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

        VkImageView defaultTexture = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, defaultTexture, 0);


        //log("Binding pipeline...");
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());

        auto now = std::chrono::high_resolution_clock::now();
        currentTime = std::chrono::duration<float>(now - startTime).count();


                //log("Getting renderable entities...");
                auto modelView = registry.view<TransformComponent, MeshComponent>();
                uint32_t modelIndex = 0;
                std::vector<std::pair<entt::entity, float>> transparentEntities;


                //log("Extracting camera pos...");
                glm::vec3 cameraPos = extractCameraPosition(view);


                //log("Rendering non transparent meshes");
                for (auto entity : modelView) {
                    auto& transform = modelView.get<TransformComponent>(entity);
                    auto& mesh = modelView.get<MeshComponent>(entity);
                    if (!mesh.isTransparent) {
                        m_p_resources->updateModelUBO(m_r_context.currentFrame, modelIndex, ModelUBO{transform.matrix(registry)});
                        auto& vulkanMesh = m_p_meshManager->getMeshByKey(modelView.get<MeshComponent>(entity).meshData.meshPath);
                        vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_r_context.currentFrame, modelIndex, currentTime, m_r_context.currentRenderResolution);
                        modelIndex++;
                    } else {
                        float distance = glm::length(transform.getWorldPosition(registry) - cameraPos);
                        transparentEntities.emplace_back(entity, distance);
                    }
                }

                std::sort(transparentEntities.begin(), transparentEntities.end(),
                          [](const auto& a, const auto& b) { return a.second > b.second; });


                //log("Drawind transparent meshes");
                for (const auto& [entity, distance] : transparentEntities) {
                    auto& transform = modelView.get<TransformComponent>(entity);
                    auto& mesh = modelView.get<MeshComponent>(entity);
                    m_p_resources->updateModelUBO(m_r_context.currentFrame, modelIndex, ModelUBO{transform.matrix(registry)});
                    auto& vulkanMesh = m_p_meshManager->getMeshByKey(modelView.get<MeshComponent>(entity).meshData.meshPath);
                    vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_r_context.currentFrame, modelIndex, currentTime, m_r_context.currentRenderResolution);
                    modelIndex++;
                }

        if (frame != 0) {
            //log("Rendering VEXUI and ImGUI...");
            //vui.render(commandBuffer, m_p_uiPipeline->get(), m_p_uiPipeline->layout(), m_r_context.currentFrame);
            //
            auto uiView = registry.view<UiComponent>();
            std::vector<UiComponent> uiObjects;
            for (auto entity : uiView) {
                if(uiView.get<UiComponent>(entity).m_vexUI->isInitialized()){
                    uiObjects.emplace_back(uiView.get<UiComponent>(entity));
                }else{
                    log("UiComponent not initialized");
                }
            }

            std::sort(uiObjects.begin(), uiObjects.end(), [](const UiComponent &f, const UiComponent &s) { return f.m_vexUI->getZIndex() < s.m_vexUI->getZIndex(); });

            for(const auto& uiObject : uiObjects) {
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
}
