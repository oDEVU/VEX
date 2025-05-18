#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace vex {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string texturePath; // Texture path from model

        void loadFromFile(const std::string& path);
        void clear() {
            vertices.clear();
            indices.clear();
            texturePath.clear();
        }
    };
}
