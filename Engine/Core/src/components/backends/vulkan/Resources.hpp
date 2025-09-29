#pragma once
#include "uniforms.hpp"
#include "context.hpp"
#include "components/errorUtils.hpp"
#include "../../VirtualFileSystem.hpp"

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

        void loadTexture(const std::string& path, const std::string& name, VirtualFileSystem *vfs);
        void unloadTexture(const std::string& name);
        VkImageView getTextureView(const std::string& name) const;

        VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {
            if (frameIndex >= m_descriptorSets.size()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid frame index %u (Max %zu)",
                           frameIndex, m_descriptorSets.size());
                return VK_NULL_HANDLE;
            }
            return m_descriptorSets[frameIndex];
        }

        VkDescriptorSet* getDescriptorSetPtr(uint32_t frameIndex) {
            return &m_descriptorSets[frameIndex];
        }
        VkDescriptorSet getUBODescriptorSet(uint32_t frameIndex) const {
            return m_descriptorSets[frameIndex];
        }
        VkDescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex);

        VkDescriptorSetLayout getDescriptorLayout() const { return m_descriptorSetLayout; }

        bool textureExists(const std::string& name) const {
            return m_textures.find(name) != m_textures.end();
        }
        uint32_t getTextureIndex(const std::string& name) const {
            auto it = m_r_context.textureIndices.find(name);
            if (it != m_r_context.textureIndices.end()) return it->second;
            return 0;
        }

    private:
        const std::string defaultTextureName = "default";

        VulkanContext& m_r_context;

        std::vector<VkBuffer> m_cameraBuffers;
        std::vector<VkBuffer> m_modelBuffers;
        std::vector<VmaAllocation> m_cameraAllocs;
        std::vector<VmaAllocation> m_modelAllocs;

        VkDescriptorSetLayout m_descriptorSetLayout;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_textureDescriptorSets;

        std::unordered_map<std::string, VkImageView> m_textures;
        VkSampler m_textureSampler;

        std::unordered_map<std::string, VkImage> m_textureImages;
        std::unordered_map<std::string, VmaAllocation> m_textureAllocations;
        std::unordered_map<std::string, VkImageView> m_textureViews;

        void createDescriptorResources();
        void createTextureSampler();
        void createPerMeshTextureSets();
    };
}
