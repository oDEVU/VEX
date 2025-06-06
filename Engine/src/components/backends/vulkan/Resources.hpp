#pragma once
#include "uniforms.hpp"
#include "context.hpp"
#include "components/errorUtils.hpp"

#include <unordered_map>
#include <string>
#include <fstream>

namespace vex {
    class VulkanResources {
    public:
        VulkanResources(VulkanContext& context);
        ~VulkanResources();

        void createUniformBuffers();

        void updateCameraUBO(const CameraUBO& data);
        void updateModelUBO(uint32_t frameIndex, uint32_t modelIndex, const ModelUBO& data);

        void createDefaultTexture();
        void updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView, uint32_t textureIndex);
        const std::string& getDefaultTextureName() const {
            return defaultTextureName;
        }

        void loadTexture(const std::string& path, const std::string& name);
        void unloadTexture(const std::string& name);
        VkImageView getTextureView(const std::string& name) const;

        VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {
            if (frameIndex >= descriptorSets_.size()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid frame index %u (Max %zu)",
                           frameIndex, descriptorSets_.size());
                return VK_NULL_HANDLE;
            }
            return descriptorSets_[frameIndex];
        }

        VkDescriptorSet* getDescriptorSetPtr(uint32_t frameIndex) {
            return &descriptorSets_[frameIndex];
        }
        VkDescriptorSet getUBODescriptorSet(uint32_t frameIndex) const {
            return descriptorSets_[frameIndex];
        }
        VkDescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex);

        VkDescriptorSetLayout getDescriptorLayout() const { return descriptorSetLayout_; }

        bool textureExists(const std::string& name) const {
            return textures_.find(name) != textures_.end();
        }
        uint32_t getTextureIndex(const std::string& name) const {
            auto it = ctx_.textureIndices.find(name);
            if (it != ctx_.textureIndices.end()) return it->second;
            return 0;
        }

    private:
        const std::string defaultTextureName = "default";

        VulkanContext& ctx_;

        std::vector<VkBuffer> cameraBuffers_;
        std::vector<VkBuffer> modelBuffers_;
        std::vector<VmaAllocation> cameraAllocs_;
        std::vector<VmaAllocation> modelAllocs_;

        VkDescriptorSetLayout descriptorSetLayout_;
        std::vector<VkDescriptorSet> descriptorSets_;
        VkDescriptorPool descriptorPool_;
        std::vector<VkDescriptorSet> textureDescriptorSets_;

        std::unordered_map<std::string, VkImageView> textures_;
        VkSampler textureSampler_;

        std::unordered_map<std::string, VkImage> textureImages_;
        std::unordered_map<std::string, VmaAllocation> textureAllocations_;
        std::unordered_map<std::string, VkImageView> textureViews_;

        void createDescriptorResources();
        void createTextureSampler();
        void createPerMeshTextureSets();
    };
}
