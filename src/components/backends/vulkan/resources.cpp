#include "resources.hpp"
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../thirdparty/stb/stb_image.h"

namespace vex {
    VulkanResources::VulkanResources(VulkanContext& context) : ctx_(context) {

        // Ensure default texture exists before descriptor updates
        createDefaultTexture();
        createTextureSampler();
        createUniformBuffers();
        createDescriptorResources();
    }

    VulkanResources::~VulkanResources() {
        // clean up Resources stuff,
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
            bufferInfo.size = sizeof(CameraUBO); // Reset size before each creation
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &cameraBuffers_[i], &cameraAllocs_[i], nullptr);

            // Model UBO (per-frame)
            bufferInfo.size = sizeof(ModelUBO); // Set explicitly
            vmaCreateBuffer(ctx_.allocator, &bufferInfo, &allocInfo,
                          &modelBuffers_[i], &modelAllocs_[i], nullptr);
        }
    }

    void VulkanResources::createDescriptorResources() {
        // Descriptor set layout
        SDL_Log("Setting up VkDescriptorSetLayoutBinding...");
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

        // Camera UBO (binding 0)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // Model UBO (binding 1)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // Texture sampler (binding 2)
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        SDL_Log("Creating up Descriptor set layout...");
        vkCreateDescriptorSetLayout(ctx_.device, &layoutInfo, nullptr, &descriptorSetLayout_);
        ctx_.descriptorSetLayout = descriptorSetLayout_;

        // Descriptor pool
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT * 2; // Camera + Model
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT * 2;
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = ctx_.MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = ctx_.MAX_FRAMES_IN_FLIGHT;

        SDL_Log("Creating up Descriptor pool...");
        vkCreateDescriptorPool(ctx_.device, &poolInfo, nullptr, &descriptorPool_);

        // Allocate descriptor sets
        std::vector<VkDescriptorSetLayout> layouts(ctx_.MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = ctx_.MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        SDL_Log("Allocating Descriptor sets...");
        descriptorSets_.resize(ctx_.MAX_FRAMES_IN_FLIGHT);
        vkAllocateDescriptorSets(ctx_.device, &allocInfo, descriptorSets_.data());

        // Update descriptor sets
        for (size_t i = 0; i < ctx_.MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            // Camera UBO (binding 0)
            VkDescriptorBufferInfo cameraBufferInfo{};
            cameraBufferInfo.buffer = cameraBuffers_[i];
            cameraBufferInfo.range = sizeof(CameraUBO);

            descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrites[0].dstSet = descriptorSets_[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &cameraBufferInfo;

            // Model UBO (binding 1)
            VkDescriptorBufferInfo modelBufferInfo{};
            modelBufferInfo.buffer = modelBuffers_[i];
            modelBufferInfo.range = sizeof(ModelUBO);

            descriptorWrites[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrites[1].dstSet = descriptorSets_[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &modelBufferInfo;

            // Texture sampler (binding 2) - Use default texture
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.sampler = textureSampler_;
            imageInfo.imageView = textures_["default"]; // Guaranteed to exist

            descriptorWrites[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrites[2].dstSet = descriptorSets_[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &imageInfo;

            SDL_Log("Updating Descriptor sets... frame: %i", i);
            vkUpdateDescriptorSets(ctx_.device,
                                 static_cast<uint32_t>(descriptorWrites.size()),
                                 descriptorWrites.data(),
                                 0, nullptr);
        }
    }


    void VulkanResources::updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = textureSampler_;
        imageInfo.imageView = textureView;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets_[frameIndex];
        write.dstBinding = 2;
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

    void VulkanResources::updateModelUBO(uint32_t frameIndex, const ModelUBO& data) {
        void* mapped;
        vmaMapMemory(ctx_.allocator, modelAllocs_[frameIndex], &mapped);
        memcpy(mapped, &data, sizeof(data));
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

        // Create image view
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
    }

    void VulkanResources::createTextureSampler() {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            SDL_Log("Creating texture sampler...");
            vkCreateSampler(ctx_.device, &samplerInfo, nullptr, &textureSampler_);
        }

        void VulkanResources::loadTexture(const std::string& path, const std::string& name) {
            std::string fullPath = "assets/" + std::string(path.c_str());

            // 1. File existence check
            std::ifstream test(fullPath);
            if (!test.is_open()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture file not found!");
                throw std::runtime_error("Missing texture: " + fullPath);
            }
            test.close();

            // Load image data
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

            // Create staging buffer
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

            // Copy pixel data to staging buffer
            void* data;
            vmaMapMemory(ctx_.allocator, stagingAlloc, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vmaUnmapMemory(ctx_.allocator, stagingAlloc);
            stbi_image_free(pixels);

            // Create Vulkan image
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

            // Transition image layout and copy buffer
            VkCommandBuffer commandBuffer = ctx_.beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = textureImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = imageInfo.extent;

            vkCmdCopyBufferToImage(commandBuffer,
                stagingBuffer,
                textureImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);

            // Transition to shader-read layout
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

            // Cleanup staging
            vmaDestroyBuffer(ctx_.allocator, stagingBuffer, stagingAlloc);

            // Create image view
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

            textures_[name] = textureView;
            textureImages_[name] = textureImage;
            textureAllocations_[name] = textureAlloc;
            textureViews_[name] = textureView;
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
