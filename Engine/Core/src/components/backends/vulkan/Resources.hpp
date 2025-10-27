/**
 *  @file   Resources.hpp
 *  @brief  This file defines VulkanResources Class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "uniforms.hpp"
#include "context.hpp"
#include "components/errorUtils.hpp"
#include "../../VirtualFileSystem.hpp"

#include <unordered_map>
#include <string>
#include <array>
#include <vector>
#include <fstream>

namespace vex {
    /// @brief This class manages resources like textures, descriptor sets, and uniform buffers.
    class VulkanResources {
    public:
        /// @brief Simple constructor.
        /// @param VulkanContext& context - The Vulkan context to use.
        /// @details Initializes the VulkanResources object with the provided Vulkan context. Its called by VulkanInterface constructor.
        VulkanResources(VulkanContext& context);

        /// @brief Simple destructor.
        ~VulkanResources();

        /// @brief Creates uniform buffers.
        void createUniformBuffers();

        /// @brief Updates the camera uniform buffer.
        /// @param const CameraUBO& data - The data to update the camera uniform buffer with.
        void updateCameraUBO(const CameraUBO& data);

        /// @brief Updates the model uniform buffer.
        /// @param uint32_t frameIndex - The frame index to update.
        /// @param uint32_t modelIndex - The model index to update.
        /// @param const ModelUBO& data - The data to update the model uniform buffer with.
        void updateModelUBO(uint32_t frameIndex, uint32_t modelIndex, const ModelUBO& data);

        /// @brief Creates a default texture.
        /// @details Creates 1x1 white texture used for untextured models.
        void createDefaultTexture();

        /// @brief Updates the texture descriptor.
        /// @param uint32_t frameIndex - The frame index to update.
        /// @param VkImageView textureView - The texture view to update.
        /// @param uint32_t textureIndex - The texture index to update.
        /// @details Updates the texture descriptor for the given frame index and texture index.
        void updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView, uint32_t textureIndex);

        /// @brief Returns the default texture name.
        /// @return std::string&
        const std::string& getDefaultTextureName() const {
            return defaultTextureName;
        }

        /// @brief Loads a texture from a file.
        /// @param const std::string& path - The path to the texture file.
        /// @param const std::string& name - The name of the texture.
        /// @param VirtualFileSystem *vfs - The virtual file system to use.
        /// @details Loads a texture from a file and creates a texture view.
        void loadTexture(const std::string& path, const std::string& name, VirtualFileSystem *vfs);

        /// @brief Unloads a texture.
        /// @param const std::string& name - The name of the texture.
        /// @details Unloads a texture and destroys its view.
        void unloadTexture(const std::string& name);

        /// @brief Returns the texture view for the given texture name.
        /// @param const std::string& name - The name of the texture.
        /// @return VkImageView - The texture view.
        VkImageView getTextureView(const std::string& name) const;

        /// @brief Creates a texture from raw data.
        /// @param const std::vector<unsigned char>& rgba - The raw data.
        /// @param int w - The width of the texture.
        /// @param int h - The height of the texture.
        /// @param const std::string& name - The name of the texture.
        /// @details Creates a texture from raw data and creates a texture view. Currently used for ui font textures.
        void createTextureFromRaw(const std::vector<unsigned char>& rgba, int w, int h, const std::string& name);

        /// @brief Returns the descriptor set for the given frame index.
        /// @param uint32_t frameIndex - The index of the frame.
        /// @return VkDescriptorSet - The descriptor set.
        VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {
            if (frameIndex >= m_descriptorSets.size()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid frame index %u (Max %zu)",
                           frameIndex, m_descriptorSets.size());
                return VK_NULL_HANDLE;
            }
            return m_descriptorSets[frameIndex];
        }

        /// @brief Returns a pointer to the descriptor set for the given frame index.
        /// @param uint32_t frameIndex - The index of the frame.
        /// @return VkDescriptorSet* - The pointer to the descriptor set.
        VkDescriptorSet* getDescriptorSetPtr(uint32_t frameIndex) {
            if (frameIndex >= m_descriptorSets.size()) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                           "Invalid frame index %u (Max %zu)",
                           frameIndex, m_descriptorSets.size());
                return VK_NULL_HANDLE;
            }
            return &m_descriptorSets[frameIndex];
        }

        /// @brief Returns a descriptor set for the given frame index and texture index.
        /// @param uint32_t frameIndex - The index of the frame.
        /// @param uint32_t textureIndex - The index of the texture.
        /// @return VkDescriptorSet - The descriptor set.
        VkDescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex);

        /// @brief Returns a descriptor set layout.
        /// @return VkDescriptorSetLayout - The descriptor set layout.
        VkDescriptorSetLayout getDescriptorLayout() const { return m_descriptorSetLayout; }

        /// @brief Returns if a texture with the given name exists.
        /// @param const std::string& name - The name of the texture.
        /// @return bool - True if the texture exists, false otherwise.
        bool textureExists(const std::string& name) const {
            return m_textures.find(name) != m_textures.end();
        }

        /// @brief Returns the index of a texture with the given name.
        /// @param const std::string& name - The name of the texture.
        /// @return uint32_t - The index of the texture.
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

        /// @brief Internal function to create descriptor resources.
        void createDescriptorResources();
        /// @brief Internal function to create texture sampler.
        void createTextureSampler();
        /// @brief Internal function to create per-mesh texture sets.
        void createPerMeshTextureSets();
    };
}
