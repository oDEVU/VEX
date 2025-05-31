#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <SDL3/SDL.h>

namespace vex {
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

        void loadFromFile(const std::string& path);
        void clear() {
            submeshes.clear();
        }
    };
}
