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

struct aiScene; // Forward declaration for Assimp scene

namespace vex {
    class VirtualFileSystem;

    /// @brief Vertex structure for mesh data
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        /// @brief UV coordinates for texture mapping but initialized to (-1, -1) for shader use. If loaded model doesnt update it engine knows it is not textured
        glm::vec2 uv = glm::vec2(-1.0f);
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

        /// @brief Loads mesh using VirtualFileSystem (supports compressed VPKs)
        void loadFromFile(const std::string& path, VirtualFileSystem* vfs);

        /// @brief Loads mesh directly from disk relative to executable (Skips VFS, useful for Editor assets)
        void loadFromRawFile(const std::string& relativePath);

        void clear() {
            submeshes.clear();
        }

    private:
        /// @brief internal helper to convert Assimp scene to Submeshes
        void processScene(const aiScene* scene, const std::string& textureBaseDir);
    };
}
