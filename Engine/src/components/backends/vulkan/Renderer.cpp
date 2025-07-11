#include "Renderer.hpp"
#include <SDL3/SDL.h>

namespace vex {
    Renderer::Renderer(VulkanContext& context,
                       std::unique_ptr<VulkanResources>& resources,
                       std::unique_ptr<VulkanPipeline>& pipeline,
                       std::unique_ptr<VulkanSwapchainManager>& swapchainManager,
                       std::unique_ptr<MeshManager>& meshManager)
        : m_r_context(context),
          m_p_resources(resources),
          m_p_pipeline(pipeline),
          m_p_swapchainManager(swapchainManager),
          m_p_meshManager(meshManager) {
        startTime = std::chrono::high_resolution_clock::now();
        log("Renderer initialized successfully");
    }

    Renderer::~Renderer() {
        log("Renderer destroyed");
    }

    void Renderer::renderFrame(const glm::mat4& view, const glm::mat4& proj, glm::uvec2 renderResolution, ImGUIWrapper& m_ui, uint64_t frame) {
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
        }

        vkResetFences(m_r_context.device, 1, &m_r_context.inFlightFences[m_r_context.currentFrame]);

        vkDeviceWaitIdle(m_r_context.device);

        m_p_resources->updateCameraUBO({view, proj});

        VkImageView textureView = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, textureView, 0);

        VkCommandBuffer commandBuffer = m_r_context.commandBuffers[m_r_context.currentImageIndex];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        transitionImageLayout(commandBuffer,
                             m_r_context.swapchainImages[m_r_context.currentImageIndex],
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             0,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);

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
        colorAttachment.clearValue.color = {{0.1f, 0.1f, 0.1f, 1.0f}};

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

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_r_context.swapchainExtent.width);
        viewport.height = static_cast<float>(m_r_context.swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_r_context.swapchainExtent.width, m_r_context.swapchainExtent.height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkImageView defaultTexture = m_p_resources->getTextureView("default");
        m_p_resources->updateTextureDescriptor(m_r_context.currentFrame, defaultTexture, 0);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_pipeline->get());

        auto now = std::chrono::high_resolution_clock::now();
        currentTime = std::chrono::duration<float>(now - startTime).count();

        auto& models = m_p_meshManager->getModels();
        for (size_t i = 0; i < m_p_meshManager->getMeshes().size(); ++i) {
            auto& vulkanMesh = m_p_meshManager->getMeshes()[i];
            m_p_resources->updateModelUBO(m_r_context.currentFrame, i, ModelUBO{models[i]->transform.matrix()});
            vulkanMesh->draw(commandBuffer, m_p_pipeline->layout(), *m_p_resources, m_r_context.currentFrame, i, currentTime, m_r_context.currentRenderResolution);
        }

        if (frame != 0) {
            m_ui.beginFrame();
            m_ui.executeUIFunctions();
            m_ui.endFrame();
        }

        vkCmdEndRendering(commandBuffer);

        transitionImageLayout(commandBuffer, m_r_context.lowResColorImage,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);

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

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            2, (VkImageMemoryBarrier[]){presentBarrier, lowResRestoreBarrier}
        );

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
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
