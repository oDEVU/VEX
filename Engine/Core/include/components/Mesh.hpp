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
    };

    /// @brief Submesh structure for mesh data, its made like this cause some file formats hold multiple meshes in one file, but engine supports only one mesh per file
    struct Submesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string texturePath;
        std::vector<glm::vec3> triangleCenters;
    };

    /// @brief Mesh data structure for loading and managing mesh data
    struct MeshData {
        std::vector<Submesh> submeshes;
        std::string meshPath;

        /// @brief Loads mesh data from a file using the Virtual File System.
        /// @details Uses Assimp with a custom `VPKAssimpIOSystem` to read files directly from VFS memory buffers (supporting compressed VPKs).
        /// Post-processes the mesh to Triangulate, Generate Normals, and Flip UVs.
        /// @param const std::string& path - Path to the mesh file within the VFS.
        /// @param VirtualFileSystem* vfs - Pointer to the active VirtualFileSystem instance.
        void loadFromFile(const std::string& path, VirtualFileSystem* vfs);

        /// @brief Loads a mesh directly from the physical disk, bypassing the VFS.
        /// @details Useful for Editor assets or raw files not yet packed into VPKs. Reads relative to the executable directory.
        /// @param const std::string& relativePath - File path relative to the executable.
        void loadFromRawFile(const std::string& relativePath);

        void clear() {
            submeshes.clear();
        }

    private:
        /// @brief Internal helper to convert an Assimp `aiScene` into VEX `Submesh` structures.
        /// @details Extracts vertices, normals, and texture coordinates. If texture coordinates are missing, initializes UVs to `(-100000.0f)` to signal the shader.
        /// @param const aiScene* scene - The loaded Assimp scene pointer.
        /// @param const std::string& textureBaseDir - Base directory string for resolving texture paths.
        void processScene(const aiScene* scene, const std::string& textureBaseDir);
    };
}
