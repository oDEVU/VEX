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

#include <components/types.hpp>
#include <string>
#include <array>
#include <vector>
#include <fstream>

namespace vex {
    /// @brief This class manages resources like textures, descriptor sets, and uniform buffers.
    class VulkanResources {
    public:
        /// @brief Simple constructor.
        /// @details Initializes the resource manager: creates default textures, samplers, UBOs, and descriptor pools.
        /// @param VulkanContext& context - The Vulkan context to use.
        /// @param VirtualFileSystem* vfs - VFS for loading texture files.
        VulkanResources(VulkanContext& context, VirtualFileSystem* vfs);

        /// @brief Simple destructor.
        /// @details Destroys all textures, buffers, samplers, and descriptor pools.
        ~VulkanResources();

        /// @brief Creates uniform buffers.
        /// @details Allocates Scene and Light UBOs for each frame in flight using VMA.
        void createUniformBuffers();

        /// @brief Updates the scene uniform buffer.
        /// @details Maps memory and copies the `SceneUBO` struct (camera, lighting, time data) to the GPU.
        /// @param const SceneUBO& data - The data to update.
        void updateSceneUBO(const SceneUBO& data);

        /// @brief Updates the light uniform buffer for a specific model.
        /// @details Updates the dynamic UBO section corresponding to a specific model index.
        /// @param uint32_t frameIndex - Current frame.
        /// @param uint32_t modelIndex - Index of the model to update lights for.
        /// @param const SceneLightsUBO& data - Light data.
        void updateLightUBO(uint32_t frameIndex, uint32_t modelIndex, const SceneLightsUBO& data);

        /// @brief Creates a default texture.
        /// @details Generates a 1x1 white pixel texture, transitions it to `SHADER_READ_ONLY`, and assigns it to the "default" key.
        void createDefaultTexture();

        /// @brief Updates the texture descriptor.
        /// @details Updates the global descriptor set (or Bindless set) to point to the new texture view.
        /// @param uint32_t frameIndex - Frame index.
        /// @param VkImageView textureView - The new view.
        /// @param uint32_t textureIndex - The slot index to update.
        void updateTextureDescriptor(uint32_t frameIndex, VkImageView textureView, uint32_t textureIndex);

        /// @brief Returns the default texture name.
        /// @return std::string&
        const std::string& getDefaultTextureName() const {
            return defaultTextureName;
        }

        /// @brief Returns the uniform buffer descriptor set for the given frame index.
        /// @param uint32_t frameIndex - The frame index.
        /// @return VkDescriptorSet - The descriptor set binding UBOs.
        VkDescriptorSet getUBODescriptorSet(uint32_t frameIndex) const;

        /// @brief Loads a texture from a file.
        /// @details Uses `stbi_load_from_memory` via VFS. Creates Vulkan Image, View, and uploads data via staging buffer. Recycles texture indices if available.
        /// @param const std::string& path - File path.
        /// @param const std::string& name - Unique identifier key.
        /// @return bool - True if loaded successfully or already exists.
        bool loadTexture(const std::string& path, const std::string& name) ;

        /// @brief Unloads a texture.
        /// @details Destroys the image/view and pushes the index to the recycled queue. Resets descriptors to the default texture.
        /// @param const std::string& name - Identifier of the texture to unload.
        void unloadTexture(const std::string& name);

        /// @brief Returns the texture view for the given texture name.
        /// @param const std::string& name - The texture identifier.
        /// @return VkImageView - The view, or the default texture view if not found.
        VkImageView getTextureView(const std::string& name) const;

        /// @brief Creates a texture from raw data.
        /// @details Creates a GPU texture from a vector of bytes (RGBA). Useful for generated content like UI fonts.
        /// @param const std::vector<unsigned char>& rgba - Raw pixel data.
        /// @param int w - Width.
        /// @param int h - Height.
        /// @param const std::string& name - Identifier.
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
        /// @param uint32_t frameIndex - Frame index.
        /// @param uint32_t textureIndex - Texture slot index.
        /// @return VkDescriptorSet - The specific descriptor set for this texture/frame combo (used in non-bindless mode).
        VkDescriptorSet getTextureDescriptorSet(uint32_t frameIndex, uint32_t textureIndex) const;

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
        /// @details If the texture isn't loaded, attempts to load it from disk using the name as the path.
        /// @param const std::string& name - Texture path/name.
        /// @return uint32_t - The texture index, or 0 (default) if loading fails.
        uint32_t getTextureIndex(const std::string& name);

        /// @brief Returns the texture sampler.
        /// @return VkSampler - The texture sampler.
        VkSampler getTextureSampler() const { return m_textureSampler; }

        /// @brief Returns the bindless descriptor set.
        /// @return VkDescriptorSet - The bindless descriptor set.
        VkDescriptorSet getBindlessDescriptorSet() const { return m_r_context.bindlessDescriptorSet; }

        /// @brief Returns the bindless descriptor set layout.
        /// @return VkDescriptorSetLayout - The bindless descriptor set layout.
        VkDescriptorSetLayout getBindlessLayout() const { return m_r_context.bindlessDescriptorSetLayout; }

    private:
        const std::string defaultTextureName = "default";

        VulkanContext& m_r_context;
        VirtualFileSystem* m_vfs;

        std::vector<VkBuffer> m_sceneBuffers;
        std::vector<VkBuffer> m_lightBuffers;
        std::vector<VmaAllocation> m_sceneAllocs;
        std::vector<VmaAllocation> m_lightAllocs;

        VkDescriptorSetLayout m_descriptorSetLayout;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::vector<VkDescriptorSet> m_textureDescriptorSets;

        vex_map<std::string, VkImageView> m_textures;
        VkSampler m_textureSampler;

        vex_map<std::string, VkImage> m_textureImages;
        vex_map<std::string, VmaAllocation> m_textureAllocations;
        vex_map<std::string, VkImageView> m_textureViews;

        std::vector<std::string> m_ignoredTexturePaths;

        /// @brief Internal function to create descriptor resources.
        void createDescriptorResources();
        /// @brief Internal function to create texture sampler.
        void createTextureSampler();
        /// @brief Internal function to create per-mesh texture sets.
        void createPerMeshTextureSets();
    };
}
