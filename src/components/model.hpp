#pragma once
#include "mesh.hpp"
#include "transform.hpp"
#include <memory>

namespace vex {
    class Model {
    public:
        MeshData meshData;
        Transform transform;
        std::vector<std::string> textureNames;  // Changed from texturePath to textureName

        void loadFromFile(const std::string& path) {
            meshData.loadFromFile(path);
            for (const auto& submesh : meshData.submeshes) {
                if (!submesh.texturePath.empty()) {
                    textureNames.push_back(submesh.texturePath);
                }
            }
        }
    };
}
