#include "Resources.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../thirdparty/stb/stb_image.h"

namespace vex {
    VulkanResources::VulkanResources(VulkanContext& context) : ctx_(context) {
        createDefaultTexture();
        createTextureSampler();
        createUniformBuffers();
        createDescriptorResources();
    }

    VulkanResources::~VulkanResources() {
        // TODO: clean up Resources stuff
    }

    void VulkanResources::createUniformBuffers() {
        SDL_Log("Creating uniform buffers...");
        cameraBuffers_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);
        cameraAllocs_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);
        modelBuffers_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);
        modelAllocs_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        for (size_t i = 0; i < ctx_.MAX_FRAMES_IN_FLIGHT; i++) {
            // Camera UBO
            bufferInfo.size = sizeof(CameraUBO);
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &cameraBuffers_[i], &cameraAllocs_[i], nullptr);

            // Model UBO (dynamic)
            bufferInfo.size = sizeof(ModelUBO) * ctx_.MAX_MODELS;
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &modelBuffers_[i], &modelAllocs_[i], nullptr);
        }
    }

    void VulkanResources::createDescriptorResources() {
        SDL_Log("Setting up VkDescriptorSetLayoutBinding...");
        std::array<VkDescriptorSetLayoutBinding, 2> uboBindings{};
        uboBindings[0].binding = 0;
        uboBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBindings[0].descriptorCount = 1;
        uboBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        uboBindings[1].binding = 1;
        uboBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        uboBindings[1].descriptorCount = 1;
        uboBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
        uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        uboLayoutInfo.bindingCount = static_cast<uint32_t>(uboBindings.size());
        uboLayoutInfo.pBindings = uboBindings.data();

        SDL_Log("Creating Uniform buffer bindings...");
        if (vkCreateDescriptorSetLayout(ctx_.device, &uboLayoutInfo, nullptr, &ctx_.uboDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
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

        SDL_Log("Creating Texture bindings...");
        if (vkCreateDescriptorSetLayout(ctx_.device, &texLayoutInfo, nullptr, &ctx_.textureDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        ctx_.descriptorSetLayout = descriptorSetLayout_;

        std::array<VkDescriptorPoolSize, 3> poolSizes{};

        // Camera UBO (static)
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT;

        // Model UBO (dynamic)
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[1].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT * ctx_.MAX_MODELS;

        // Textures
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT * ctx_.MAX_TEXTURES;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = ctx_.MAX_FRAMES_IN_FLIGHT * (2 + ctx_.MAX_TEXTURES);

        SDL_Log("Creating descriptor pool with %d max sets", poolInfo.maxSets);
        if (vkCreateDescriptorPool(ctx_.device, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }

        std::vector<VkDescriptorSetLayout> uboLayouts(ctx_.MAX_FRAMES_IN_FLIGHT, ctx_.uboDescriptorSetLayout);
        VkDescriptorSetAllocateInfo uboAllocInfo{};
        uboAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        uboAllocInfo.descriptorPool = descriptorPool_;
        uboAllocInfo.descriptorSetCount = ctx_.MAX_FRAMES_IN_FLIGHT;
        uboAllocInfo.pSetLayouts = uboLayouts.data();

        SDL_Log("Allocating %d UBO descriptor sets", ctx_.MAX_FRAMES_IN_FLIGHT);
        descriptorSets_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(ctx_.device, &uboAllocInfo, descriptorSets_.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate UBO descriptor sets");
        }

        createPerMeshTextureSets();
        for (size_t i = 0; i < ctx_.MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 2> uboWrites{};

            // Camera UBO
            VkDescriptorBufferInfo cameraBufferInfo{};
            cameraBufferInfo.buffer = cameraBuffers_[i];
            cameraBufferInfo.range = sizeof(CameraUBO);

            uboWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            uboWrites[0].dstSet = descriptorSets_[i];
            uboWrites[0].dstBinding = 0;
            uboWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrites[0].descriptorCount = 1;
            uboWrites[0].pBufferInfo = &cameraBufferInfo;

            // Model UBO
            VkDescriptorBufferInfo modelBufferInfo{};
            modelBufferInfo.buffer = modelBuffers_[i];
            modelBufferInfo.range = sizeof(ModelUBO);

            uboWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            uboWrites[1].dstSet = descriptorSets_[i];
            uboWrites[1].dstBinding = 1;
            uboWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            uboWrites[1].descriptorCount = 1;
            uboWrites[1].pBufferInfo = &modelBufferInfo;

            vkUpdateDescriptorSets(ctx_.device, uboWrites.size(), uboWrites.data(), 0, nullptr);
        }
    }

    void VulkanResources::updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView, uint32_t textureIndex){
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = textureSampler_;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = getTextureDescriptorSet(frameIndex, textureIndex);
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(ctx_.device, 1, &write, 0, nullptr);
    }

    void VulkanResources::updateCameraUBO(const CameraUBO& data) {
        void* mapped;
        vmaMapMemory(ctx_.allocator, cameraAllocs_[ctx_.currentFrame], &mapped);
        memcpy(mapped, &data, sizeof(data));
        vmaUnmapMemory(ctx_.allocator, cameraAllocs_[ctx_.currentFrame]);
    }

    void VulkanResources::updateModelUBO(uint32_t frameIndex, uint32_t modelIndex, const ModelUBO& data) {
        if (modelIndex >= ctx_.MAX_MODELS) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "Model index %u exceeds MAX_MODELS (%u)",
                         modelIndex, ctx_.MAX_MODELS);
            return;
        }
        void* mapped;
        vmaMapMemory(ctx_.allocator, modelAllocs_[frameIndex], &mapped);
        char* modelData = static_cast<char*>(mapped) + modelIndex * sizeof(ModelUBO);
        memcpy(modelData, &data, sizeof(ModelUBO));
        vmaUnmapMemory(ctx_.allocator, modelAllocs_[frameIndex]);
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
        SDL_Log("Creating default texture image...");
        if (vmaCreateImage(ctx_.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create default texture image");
        }

        VkCommandBuffer commandBuffer = ctx_.beginSingleTimeCommands();

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

        ctx_.endSingleTimeCommands(commandBuffer);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        SDL_Log("Creating default texture image view...");
        if (vkCreateImageView(ctx_.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            vmaDestroyImage(ctx_.allocator, image, allocation);
            throw std::runtime_error("Failed to create default texture image view");
        }

        textures_["default"] = imageView;
        textureImages_["default"] = image;
        textureAllocations_["default"] = allocation;
        textureViews_["default"] = imageView;

        ctx_.textureIndices["default"] = 0;
        ctx_.nextTextureIndex = 1;
        SDL_Log("Created default texture with index 0");
    }

    void VulkanResources::createPerMeshTextureSets() {
        SDL_Log("Creating %d texture descriptor sets", ctx_.MAX_FRAMES_IN_FLIGHT * ctx_.MAX_TEXTURES);

        textureDescriptorSets_.resize(ctx_.MAX_FRAMES_IN_FLIGHT * ctx_.MAX_TEXTURES);

        std::vector<VkDescriptorSetLayout> layouts(
            ctx_.MAX_FRAMES_IN_FLIGHT * ctx_.MAX_TEXTURES,
            ctx_.textureDescriptorSetLayout
        );

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        VkResult result = vkAllocateDescriptorSets(ctx_.device, &allocInfo, textureDescriptorSets_.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate texture descriptor sets: " + std::to_string(result));
        }

        VkDescriptorImageInfo defaultImageInfo{};
        defaultImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        defaultImageInfo.sampler = textureSampler_;
        defaultImageInfo.imageView = getTextureView("default");

        std::vector<VkWriteDescriptorSet> writes;
        for (uint32_t frame = 0; frame < ctx_.MAX_FRAMES_IN_FLIGHT; ++frame) {
            for (uint32_t texIdx = 0; texIdx < ctx_.MAX_TEXTURES; ++texIdx) {
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
        vkUpdateDescriptorSets(ctx_.device, writes.size(), writes.data(), 0, nullptr);
        SDL_Log("Initialized %d texture descriptor sets with default texture", writes.size());
    }


    VkDescriptorSet VulkanResources::getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex) {
        const uint32_t index = frameIndex * ctx_.MAX_TEXTURES + textureIndex;
        if (index >= textureDescriptorSets_.size()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                       "Texture index out of bounds (Frame: %u, TexIndex: %u, Max: %u)",
                       frameIndex, textureIndex, ctx_.MAX_TEXTURES);
            return VK_NULL_HANDLE;
        }
        return textureDescriptorSets_[index];
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

            SDL_Log("Creating texture sampler...");
            vkCreateSampler(ctx_.device, &samplerInfo, nullptr, &textureSampler_);
        }

        void VulkanResources::loadTexture(const std::string& path, const std::string& name) {
            std::string fullPath = "Assets/" + std::string(path.c_str());

            if (ctx_.textureIndices.size() >= ctx_.MAX_TEXTURES) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Maximum texture count (%u) reached!",
                           ctx_.MAX_TEXTURES);
                return;
            }
            std::ifstream test(fullPath);
            if (!test.is_open()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture file not found!");
                throw std::runtime_error("Missing texture: " + fullPath);
            }
            test.close();

            SDL_Log("STBI loading image...");
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "STBI failed: %s", stbi_failure_reason());
                throw std::runtime_error("Failed to load texture pixels");
            }
            SDL_Log("Image loaded: %dx%d, %d channels", texWidth, texHeight, texChannels);

            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) {
                throw std::runtime_error("Failed to load texture: " + fullPath);
            }

            SDL_Log("Creating staging buffer (%zu bytes)...",
                  static_cast<size_t>(texWidth * texHeight * 4));
            VkBuffer stagingBuffer;
            VmaAllocation stagingAlloc;
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = imageSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);

            void* data;
            vmaMapMemory(ctx_.allocator, stagingAlloc, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vmaUnmapMemory(ctx_.allocator, stagingAlloc);
            stbi_image_free(pixels);

            SDL_Log("Creating Vulkan image...");
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
            vmaCreateImage(ctx_.allocator, &imageInfo, &imageAllocInfo, &textureImage, &textureAlloc, nullptr);

            VkCommandBuffer commandBuffer = ctx_.beginSingleTimeCommands();

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

            ctx_.endSingleTimeCommands(commandBuffer);

            vkDeviceWaitIdle(ctx_.device);

            vmaDestroyBuffer(ctx_.allocator, stagingBuffer, stagingAlloc);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = textureImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView textureView;
            SDL_Log("Creating vulkan texture view...");
            if (vkCreateImageView(ctx_.device, &viewInfo, nullptr, &textureView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create texture image view!");
            }

            if (ctx_.textureIndices.contains(name)) {
                SDL_Log("Texture '%s' already exists at index %u", name.c_str(), ctx_.textureIndices[name]);
                return;
            }

            ctx_.textureIndices[name] = ctx_.nextTextureIndex++;
            SDL_Log("Assigned texture '%s' to index %u", name.c_str(), ctx_.textureIndices[name]);

            textures_[name] = textureView;
            textureImages_[name] = textureImage;
            textureAllocations_[name] = textureAlloc;
            textureViews_[name] = textureView;

                if (ctx_.textureIndices.size() >= ctx_.MAX_TEXTURES) {
                    throw std::runtime_error("Exceeded maximum texture count");
                }
                ctx_.textureIndices[name] = ctx_.textureIndices.size();
                SDL_Log("Assigned texture '%s' to index %d", name.c_str(), ctx_.textureIndices[name]);

                VkDescriptorImageInfo imageDescInfo{};
                imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageDescInfo.sampler = textureSampler_;
                imageDescInfo.imageView = textureView;

                for (uint32_t frame = 0; frame < ctx_.MAX_FRAMES_IN_FLIGHT; ++frame) {
                    VkWriteDescriptorSet write{};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet = getTextureDescriptorSet(frame, ctx_.textureIndices[name]);
                    write.dstBinding = 0;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    write.descriptorCount = 1;
                    write.pImageInfo = &imageDescInfo;

                    vkUpdateDescriptorSets(ctx_.device, 1, &write, 0, nullptr);
                }
                SDL_Log("Updated texture descriptors for '%s'", name.c_str());
        }
        void VulkanResources::unloadTexture(const std::string& name) {
            if (name == "default") return;

            auto it = textures_.find(name);
            if (it == textures_.end()) return;

            vkDestroyImageView(ctx_.device, textureViews_[name], nullptr);
            vmaDestroyImage(ctx_.allocator, textureImages_[name], textureAllocations_[name]);

            textures_.erase(name);
            textureImages_.erase(name);
            textureAllocations_.erase(name);
            textureViews_.erase(name);
        }

        VkImageView VulkanResources::getTextureView(const std::string& name) const {
            auto it = textures_.find(name);
            return (it != textures_.end()) ? it->second : textures_.at(getDefaultTextureName());
        }
}
