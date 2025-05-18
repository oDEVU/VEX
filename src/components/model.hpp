#pragma once
#include "mesh.hpp"
#include "transform.hpp"
#include <memory>

namespace vex {
    class Model {
    public:
        MeshData meshData;
        Transform transform;
        std::string textureName;  // Changed from texturePath to textureName

        void loadFromFile(const std::string& path) {
            meshData.loadFromFile(path);
            // If you want to keep texture path separate
            textureName = path;  // Or use meshData.texturePath if needed
        }
    };
}
