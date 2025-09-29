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

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv = glm::vec2(-1.0f);
    };

    struct Submesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string texturePath;
    };

    struct MeshData {
        std::vector<Submesh> submeshes;
        std::string meshPath;
        void loadFromFile(const std::string& path, VirtualFileSystem* vfs);
        void clear() {
            submeshes.clear();
        }
    };
}
