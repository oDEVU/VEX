#include "Resources.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_vulkan.h>
#include "components/backends/vulkan/uniforms.hpp"
#include "limits.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../thirdparty/stb/stb_image.h"

namespace vex {
    VulkanResources::VulkanResources(VulkanContext& context) : m_r_context(context) {
        createDefaultTexture();
        createTextureSampler();
        createUniformBuffers();
        createDescriptorResources();
    }

    VulkanResources::~VulkanResources() {
        vkDeviceWaitIdle(m_r_context.device);

        if (m_textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_r_context.device, m_textureSampler, nullptr);
            m_textureSampler = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            if (m_sceneBuffers[i] != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, m_sceneBuffers[i], m_sceneAllocs[i]);
                m_sceneBuffers[i] = VK_NULL_HANDLE;
            }
            if (m_lightBuffers[i] != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_r_context.allocator, m_lightBuffers[i], m_lightAllocs[i]);
                m_lightBuffers[i] = VK_NULL_HANDLE;
            }
        }

        for (auto& [name, image] : m_textureImages) {
            if (m_textureViews[name] != VK_NULL_HANDLE) {
                vkDestroyImageView(m_r_context.device, m_textureViews[name], nullptr);
                m_textureViews[name] = VK_NULL_HANDLE;
            }
            if (image != VK_NULL_HANDLE && m_textureAllocations[name] != VK_NULL_HANDLE) {
                vmaDestroyImage(m_r_context.allocator, image, m_textureAllocations[name]);
                image = VK_NULL_HANDLE;
            }
        }

        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_r_context.device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }
    }

    void VulkanResources::createTextureFromRaw(const std::vector<unsigned char>& rgba, int w, int h, const std::string& name) {
        // Copy the loadTexture logic, but use rgba instead of stb_image
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;

        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = rgba.size();
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        vmaCreateBuffer(m_r_context.allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, nullptr);

        void* data;
        vmaMapMemory(m_r_context.allocator, stagingAlloc, &data);
        memcpy(data, rgba.data(), rgba.size());
        vmaUnmapMemory(m_r_context.allocator, stagingAlloc);

        VkImage textureImage;
        VmaAllocation textureAlloc;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = w;
        imageInfo.extent.height = h;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(m_r_context.allocator, &imageInfo, &allocInfo, &textureImage, &textureAlloc, nullptr);

        VkCommandBuffer commandBuffer = m_r_context.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = textureImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        m_r_context.endSingleTimeCommands(commandBuffer);

        vmaDestroyBuffer(m_r_context.allocator, stagingBuffer, stagingAlloc);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView textureView;
        log("Creating vulkan texture view...");
        if (vkCreateImageView(m_r_context.device, &viewInfo, nullptr, &textureView) != VK_SUCCESS) {
            throw_error("Failed to create texture image view!");
        }

        if (m_r_context.textureIndices.contains(name)) {
            log("Texture '%s' already exists at index %u", name.c_str(), m_r_context.textureIndices[name]);
            return;
        }

        m_r_context.textureIndices[name] = m_r_context.nextTextureIndex++;
        log("Assigned texture '%s' to index %u", name.c_str(), m_r_context.textureIndices[name]);

        m_textures[name] = textureView;
        m_textureImages[name] = textureImage;
        m_textureAllocations[name] = textureAlloc;
        m_textureViews[name] = textureView;

            if (m_r_context.textureIndices.size() >= MAX_TEXTURES) {
                throw_error("Exceeded maximum texture count");
            }
            m_r_context.textureIndices[name] = m_r_context.textureIndices.size();
            log("Assigned texture '%s' to index %d", name.c_str(), m_r_context.textureIndices[name]);

        VkDescriptorImageInfo imageDescInfo{};
        imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescInfo.sampler = m_textureSampler;
        imageDescInfo.imageView = textureView;

        for (uint32_t frame = 0; frame < m_r_context.MAX_FRAMES_IN_FLIGHT; ++frame) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = getTextureDescriptorSet(frame, m_r_context.textureIndices[name]);
            write.dstBinding = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageDescInfo;

            vkUpdateDescriptorSets(m_r_context.device, 1, &write, 0, nullptr);
        }
    }

    void VulkanResources::createUniformBuffers() {
        log("Creating uniform buffers...");
        m_sceneBuffers.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_sceneAllocs.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_lightBuffers.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_lightAllocs.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        for (size_t i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            // scene UBO
            bufferInfo.size = sizeof(SceneUBO);
            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                          &m_sceneBuffers[i], &m_sceneAllocs[i], nullptr);

            // Light UBO (dynamic)
            bufferInfo.size = sizeof(SceneLightsUBO) * MAX_MODELS;
            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo,
                            &m_lightBuffers[i], &m_lightAllocs[i], nullptr);
        }
    }

    void VulkanResources::createDescriptorResources() {
        log("Setting up VkDescriptorSetLayoutBinding...");
        std::array<VkDescriptorSetLayoutBinding, 2> uboBindings{};
        uboBindings[0].binding = 0;
        uboBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBindings[0].descriptorCount = 1;
        uboBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        uboBindings[1].binding = 1;
        uboBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        uboBindings[1].descriptorCount = 1;
        uboBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
        uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        uboLayoutInfo.bindingCount = static_cast<uint32_t>(uboBindings.size());
        uboLayoutInfo.pBindings = uboBindings.data();

        log("Creating Uniform buffer bindings...");
        if (vkCreateDescriptorSetLayout(m_r_context.device, &uboLayoutInfo, nullptr, &m_r_context.uboDescriptorSetLayout) != VK_SUCCESS) {
            throw_error("Failed to create descriptor set layout");
        }

    VkDescriptorSetLayoutBinding texBinding{};
    texBinding.binding = 0;
    texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.descriptorCount = 1;
    texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texLayoutInfo{};
    texLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texLayoutInfo.bindingCount = 1;
    texLayoutInfo.pBindings = &texBinding;

        log("Creating Texture bindings...");
        if (vkCreateDescriptorSetLayout(m_r_context.device, &texLayoutInfo, nullptr, &m_r_context.textureDescriptorSetLayout) != VK_SUCCESS) {
            throw_error("Failed to create descriptor set layout");
        }

        m_r_context.descriptorSetLayout = m_descriptorSetLayout;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};

        // Scene UBO (static)
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = m_r_context.MAX_FRAMES_IN_FLIGHT;

        // Light UBO (dynamic)
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[1].descriptorCount = m_r_context.MAX_FRAMES_IN_FLIGHT * MAX_MODELS;

        // Textures
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = m_r_context.MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = m_r_context.MAX_FRAMES_IN_FLIGHT * (3 + MAX_TEXTURES);

        log("Creating descriptor pool with %d max sets", poolInfo.maxSets);
        if (vkCreateDescriptorPool(m_r_context.device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw_error("Failed to create descriptor pool");
        }

        std::vector<VkDescriptorSetLayout> uboLayouts(m_r_context.MAX_FRAMES_IN_FLIGHT, m_r_context.uboDescriptorSetLayout);
        VkDescriptorSetAllocateInfo uboAllocInfo{};
        uboAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        uboAllocInfo.descriptorPool = m_descriptorPool;
        uboAllocInfo.descriptorSetCount = m_r_context.MAX_FRAMES_IN_FLIGHT;
        uboAllocInfo.pSetLayouts = uboLayouts.data();

        log("Allocating %d UBO descriptor sets", m_r_context.MAX_FRAMES_IN_FLIGHT);
        m_descriptorSets.resize(m_r_context.MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_r_context.device, &uboAllocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
            throw_error("Failed to allocate UBO descriptor sets");
        }

        createPerMeshTextureSets();
        for (size_t i = 0; i < m_r_context.MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 2> uboWrites{};

            // Scene UBO
            VkDescriptorBufferInfo sceneBufferInfo{};
            sceneBufferInfo.buffer = m_sceneBuffers[i];
            sceneBufferInfo.range = sizeof(SceneUBO);

            uboWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            uboWrites[0].dstSet = m_descriptorSets[i];
            uboWrites[0].dstBinding = 0;
            uboWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrites[0].descriptorCount = 1;
            uboWrites[0].pBufferInfo = &sceneBufferInfo;

            // Light UBO
            VkDescriptorBufferInfo lightInfo{};
            lightInfo.buffer = m_lightBuffers[i];
            lightInfo.range = sizeof(SceneLightsUBO);

            uboWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            uboWrites[1].dstSet = m_descriptorSets[i];
            uboWrites[1].dstBinding = 1;
            uboWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            uboWrites[1].descriptorCount = 1;
            uboWrites[1].pBufferInfo = &lightInfo;

            vkUpdateDescriptorSets(m_r_context.device, uboWrites.size(), uboWrites.data(), 0, nullptr);
        }
    }

    VkDescriptorSet VulkanResources::getUBODescriptorSet(uint32_t frameIndex) const {
        if (frameIndex >= m_descriptorSets.size()) return VK_NULL_HANDLE;
        return m_descriptorSets[frameIndex];
    }

    void VulkanResources::updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView, uint32_t textureIndex){
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = m_textureSampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = getTextureDescriptorSet(frameIndex, textureIndex);
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_r_context.device, 1, &write, 0, nullptr);
    }

    void VulkanResources::updateSceneUBO(const SceneUBO& data) {
        void* mapped;
        vmaMapMemory(m_r_context.allocator, m_sceneAllocs[m_r_context.currentFrame], &mapped);
        memcpy(mapped, &data, sizeof(data));
        vmaUnmapMemory(m_r_context.allocator, m_sceneAllocs[m_r_context.currentFrame]);
    }

    void VulkanResources::updateLightUBO(uint32_t frameIndex, uint32_t modelIndex, const SceneLightsUBO& data) {
        if (modelIndex >= MAX_MODELS) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "Model index %u exceeds MAX_MODELS (%u)",
                         modelIndex, MAX_MODELS);
            return;
        }
        void* mapped;
        vmaMapMemory(m_r_context.allocator, m_lightAllocs[frameIndex], &mapped);
        char* lightData = static_cast<char*>(mapped) + modelIndex * sizeof(SceneLightsUBO);
        memcpy(lightData, &data, sizeof(SceneLightsUBO));
        vmaUnmapMemory(m_r_context.allocator, m_lightAllocs[frameIndex]);
    }

    void VulkanResources::createDefaultTexture() {
        // Create a 1x1 white pixel texture
        const unsigned char pixels[] = {255, 255, 255, 255};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {1, 1, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage image;
        VmaAllocation allocation;
        log("Creating default texture image...");
        if (vmaCreateImage(m_r_context.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
            throw_error("Failed to create default texture image");
        }

        VkCommandBuffer commandBuffer = m_r_context.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        m_r_context.endSingleTimeCommands(commandBuffer);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        log("Creating default texture image view...");
        if (vkCreateImageView(m_r_context.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            vmaDestroyImage(m_r_context.allocator, image, allocation);
            throw_error("Failed to create default texture image view");
        }

        m_textures["default"] = imageView;
        m_textureImages["default"] = image;
        m_textureAllocations["default"] = allocation;
        m_textureViews["default"] = imageView;

        m_r_context.textureIndices["default"] = 0;
        m_r_context.nextTextureIndex = 1;
        log("Created default texture with index 0");
    }

    void VulkanResources::createPerMeshTextureSets() {
        log("Creating %d texture descriptor sets", m_r_context.MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES);

        m_textureDescriptorSets.resize(m_r_context.MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES);

        std::vector<VkDescriptorSetLayout> layouts(
            m_r_context.MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES,
            m_r_context.textureDescriptorSetLayout
        );

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        VkResult result = vkAllocateDescriptorSets(m_r_context.device, &allocInfo, m_textureDescriptorSets.data());
        if (result != VK_SUCCESS) {
            throw_error("Failed to allocate texture descriptor sets: " + std::to_string(result));
        }

        VkDescriptorImageInfo defaultImageInfo{};
        defaultImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        defaultImageInfo.sampler = m_textureSampler;
        defaultImageInfo.imageView = getTextureView("default");

        std::vector<VkWriteDescriptorSet> writes;
        for (uint32_t frame = 0; frame < m_r_context.MAX_FRAMES_IN_FLIGHT; ++frame) {
            for (uint32_t texIdx = 0; texIdx < MAX_TEXTURES; ++texIdx) {
                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = getTextureDescriptorSet(frame, texIdx);
                write.dstBinding = 0;
                write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write.descriptorCount = 1;
                write.pImageInfo = &defaultImageInfo;
                writes.push_back(write);
            }
        }
        vkUpdateDescriptorSets(m_r_context.device, writes.size(), writes.data(), 0, nullptr);
        log("Initialized %d texture descriptor sets with default texture", writes.size());
    }


    VkDescriptorSet VulkanResources::getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex) {
        const uint32_t index = frameIndex * MAX_TEXTURES + textureIndex;
        if (index >= m_textureDescriptorSets.size()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                       "Texture index out of bounds (Frame: %u, TexIndex: %u, Max: %u)",
                       frameIndex, textureIndex, MAX_TEXTURES);
            return VK_NULL_HANDLE;
        }
        return m_textureDescriptorSets[index];
    }

    void VulkanResources::createTextureSampler() {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

            log("Creating texture sampler...");
            vkCreateSampler(m_r_context.device, &samplerInfo, nullptr, &m_textureSampler);
        }

        void VulkanResources::loadTexture(const std::string& path, const std::string& name, VirtualFileSystem *vfs) {
            std::string fullPath = path;//"Assets/" + std::string(path.c_str());

            if (m_r_context.textureIndices.size() >= MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Maximum texture count (%u) reached!",
                           MAX_TEXTURES);
                return;
            }

                // Use VFS to check if file exists and get data
                if (!vfs->file_exists(fullPath)) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture file not found in VFS: %s", fullPath.c_str());
                    throw_error("Missing texture in VFS: " + fullPath);
                }

                log("VFS loading image...");
                int texWidth, texHeight, texChannels;

                // Load file data through VFS
                auto fileData = vfs->load_file(fullPath);  // This returns std::unique_ptr<FileData>
                if (!fileData) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VFS failed to load texture: %s", fullPath.c_str());
                    throw_error("VFS failed to load texture: " + fullPath);
                }

                // Use stbi_load_from_memory instead of stbi_load
                stbi_uc* pixels = stbi_load_from_memory(
                    reinterpret_cast<const stbi_uc*>(fileData->data.data()),  // fileData is a pointer
                    static_cast<int>(fileData->size),                         // fileData->size is member
                    &texWidth, &texHeight, &texChannels, STBI_rgb_alpha
                );

                if (!pixels) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "STBI failed: %s", stbi_failure_reason());
                    throw_error("Failed to load texture pixels from VFS data");
                }

                log("Image loaded from VFS: %dx%d, %d channels", texWidth, texHeight, texChannels);

                VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) {
                throw_error("Failed to load texture: " + fullPath);
            }

            log("Creating staging buffer (%zu bytes)...",
                  static_cast<size_t>(texWidth * texHeight * 4));
            VkBuffer stagingBuffer;
            VmaAllocation stagingAlloc;
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = imageSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            vmaCreateBuffer(m_r_context.allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);

            void* data;
            vmaMapMemory(m_r_context.allocator, stagingAlloc, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vmaUnmapMemory(m_r_context.allocator, stagingAlloc);
            stbi_image_free(pixels);

            log("Creating Vulkan image...");
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            VmaAllocationCreateInfo imageAllocInfo{};
            imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkImage textureImage;
            VmaAllocation textureAlloc;
            vmaCreateImage(m_r_context.allocator, &imageInfo, &imageAllocInfo, &textureImage, &textureAlloc, nullptr);

            VkCommandBuffer commandBuffer = m_r_context.beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = textureImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;


            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = imageInfo.extent;


            vkCmdCopyBufferToImage(commandBuffer,
                stagingBuffer,
                textureImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            m_r_context.endSingleTimeCommands(commandBuffer);

            vkDeviceWaitIdle(m_r_context.device);

            vmaDestroyBuffer(m_r_context.allocator, stagingBuffer, stagingAlloc);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = textureImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView textureView;
            log("Creating vulkan texture view...");
            if (vkCreateImageView(m_r_context.device, &viewInfo, nullptr, &textureView) != VK_SUCCESS) {
                throw_error("Failed to create texture image view!");
            }

            if (m_r_context.textureIndices.contains(name)) {
                log("Texture '%s' already exists at index %u", name.c_str(), m_r_context.textureIndices[name]);
                return;
            }

            m_r_context.textureIndices[name] = m_r_context.nextTextureIndex++;
            log("Assigned texture '%s' to index %u", name.c_str(), m_r_context.textureIndices[name]);

            m_textures[name] = textureView;
            m_textureImages[name] = textureImage;
            m_textureAllocations[name] = textureAlloc;
            m_textureViews[name] = textureView;

                if (m_r_context.textureIndices.size() >= MAX_TEXTURES) {
                    throw_error("Exceeded maximum texture count");
                }
                m_r_context.textureIndices[name] = m_r_context.textureIndices.size();
                log("Assigned texture '%s' to index %d", name.c_str(), m_r_context.textureIndices[name]);

                VkDescriptorImageInfo imageDescInfo{};
                imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageDescInfo.sampler = m_textureSampler;
                imageDescInfo.imageView = textureView;

                for (uint32_t frame = 0; frame < m_r_context.MAX_FRAMES_IN_FLIGHT; ++frame) {
                    VkWriteDescriptorSet write{};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet = getTextureDescriptorSet(frame, m_r_context.textureIndices[name]);
                    write.dstBinding = 0;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    write.descriptorCount = 1;
                    write.pImageInfo = &imageDescInfo;

                    vkUpdateDescriptorSets(m_r_context.device, 1, &write, 0, nullptr);
                }
                log("Updated texture descriptors for '%s'", name.c_str());
        }
        void VulkanResources::unloadTexture(const std::string& name) {
            if (name == "default") return;

            auto it = m_textures.find(name);
            if (it == m_textures.end()){
                log("Texture %s not loaded", name.c_str());
                return;
            }

            vkDeviceWaitIdle(m_r_context.device);

            if (m_textureViews[name] != VK_NULL_HANDLE) {
                vkDestroyImageView(m_r_context.device, m_textureViews[name], nullptr);
                m_textureViews[name] = VK_NULL_HANDLE;
            }

            if (m_textureImages[name] != VK_NULL_HANDLE && m_textureAllocations[name] != VK_NULL_HANDLE) {
                vmaDestroyImage(m_r_context.allocator, m_textureImages[name], m_textureAllocations[name]);
                m_textureImages[name] = VK_NULL_HANDLE;
            }

            m_textures.erase(name);
            m_textureImages.erase(name);
            m_textureAllocations.erase(name);
            m_textureViews.erase(name);
            log("Texture %s unloaded", name.c_str());
        }

        VkImageView VulkanResources::getTextureView(const std::string& name) const {
            auto it = m_textures.find(name);
            return (it != m_textures.end()) ? it->second : m_textures.at(getDefaultTextureName());
        }
}
