#pragma once
#include "uniforms.hpp"
#include "context.hpp"
#include <unordered_map>
#include <string>

namespace vex {
    class VulkanResources {
    public:
        VulkanResources(VulkanContext& context);
        ~VulkanResources();

        // Buffer management
        void createUniformBuffers();

        void updateCameraUBO(const CameraUBO& data);
        void updateModelUBO(uint32_t frameIndex, const ModelUBO& data);

        void createDefaultTexture();
        void updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView);
        const std::string& getDefaultTextureName() const {
            return "default";  // Return const reference
        }

        // Texture management
        void loadTexture(const std::string& path, const std::string& name);
        void unloadTexture(const std::string& name);
        VkImageView getTextureView(const std::string& name) const;

        VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const;

        VkDescriptorSet* getDescriptorSetPtr(uint32_t frameIndex) {
            return &descriptorSets_[frameIndex];
        }

        VkDescriptorSetLayout getDescriptorLayout() const { return descriptorSetLayout_; }

        bool textureExists(const std::string& name) const {
            return textures_.find(name) != textures_.end();
        }

    private:
        VulkanContext& ctx_;

        // Uniform buffers
        std::vector<VkBuffer> cameraBuffers_;
        std::vector<VkBuffer> modelBuffers_;
        std::vector<VmaAllocation> cameraAllocs_;
        std::vector<VmaAllocation> modelAllocs_;

        // Descriptors
        VkDescriptorSetLayout descriptorSetLayout_;
        std::vector<VkDescriptorSet> descriptorSets_;
        VkDescriptorPool descriptorPool_;

        // Textures
        std::unordered_map<std::string, VkImageView> textures_;
        VkSampler textureSampler_;

        std::unordered_map<std::string, VkImage> textureImages_;
        std::unordered_map<std::string, VmaAllocation> textureAllocations_;
        std::unordered_map<std::string, VkImageView> textureViews_;

        void createDescriptorResources();
        void createTextureSampler();
    };
}
