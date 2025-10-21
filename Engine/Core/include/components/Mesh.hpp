/**
 *  @file   Mesh.hpp
 *  @brief  This file defines MeshData struct and all other structs needed for it.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/pathUtils.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <iostream>

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
    };

    /// @brief Mesh data structure for loading and managing mesh data
    struct MeshData {
        std::vector<Submesh> submeshes;
        std::string meshPath;
        void loadFromFile(const std::string& path, VirtualFileSystem* vfs);
        void clear() {
            submeshes.clear();
        }
    };
}
