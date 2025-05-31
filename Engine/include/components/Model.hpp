#pragma once
#include "Mesh.hpp"
#include "transform.hpp"
#include <memory>

namespace vex {
    class Model {
    public:
        MeshData meshData;
        Transform transform;
        uint32_t id = UINT32_MAX;
        std::vector<std::string> textureNames;

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
