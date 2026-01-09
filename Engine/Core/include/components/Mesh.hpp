/**
 * @file   Mesh.hpp
 * @brief  This file defines MeshData struct and all other structs needed for it.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/pathUtils.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <iostream>

struct aiScene;

namespace vex {
    class VirtualFileSystem;

    /// @brief Vertex structure for mesh data
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        /// @brief UV coordinates for texture mapping but initialized to (-100000, -100000) for shader use. If loaded model doesnt update it engine knows it is not textured
        glm::vec2 uv = glm::vec2(-100000.0f);
        uint32_t textureIndex = 0;
    };

    /// @brief Submesh range structure for mesh data
    struct SubmeshRange {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t textureIndex;
    };

    /// @brief Mesh data structure for loading and managing mesh data
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        //std::string texturePath;
        std::vector<std::string> texturePaths;
        std::vector<SubmeshRange> submeshes;
        std::string meshPath;

<<<<<<< Updated upstream
        /// @brief Loads mesh using VirtualFileSystem (supports compressed VPKs)
=======
        /// @brief Clears all mesh data
        void clear(){
            vertices.clear();
            indices.clear();
            texturePaths.clear();
            submeshes.clear();
            meshPath.clear();
        }

        /// @brief Loads mesh data from a file using the Virtual File System.
        /// @details Uses Assimp with a custom `VPKAssimpIOSystem` to read files directly from VFS memory buffers (supporting compressed VPKs).
        /// Post-processes the mesh to Triangulate, Generate Normals, and Flip UVs.
        /// @param const std::string& path - Path to the mesh file within the VFS.
        /// @param VirtualFileSystem* vfs - Pointer to the active VirtualFileSystem instance.
>>>>>>> Stashed changes
        void loadFromFile(const std::string& path, VirtualFileSystem* vfs);

        /// @brief Loads mesh directly from disk relative to executable (Skips VFS, useful for Editor assets)
        void loadFromRawFile(const std::string& relativePath);

    private:
        /// @brief internal helper to convert Assimp scene to Submeshes
        void processScene(const aiScene* scene, const std::string& textureBaseDir);
    };
}
