#include "mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>

namespace vex {
    void MeshData::loadFromFile(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("Failed to load model: " + std::string(importer.GetErrorString()));
        }

        // Process first mesh only for simplicity
        aiMesh* aiMesh = scene->mMeshes[0];

        // Vertices
        vertices.resize(aiMesh->mNumVertices);
        for (unsigned i = 0; i < aiMesh->mNumVertices; i++) {
            vertices[i].position = {
                aiMesh->mVertices[i].x,
                aiMesh->mVertices[i].y,
                aiMesh->mVertices[i].z
            };

            vertices[i].normal = {
                aiMesh->mNormals[i].x,
                aiMesh->mNormals[i].y,
                aiMesh->mNormals[i].z
            };

            if (aiMesh->mTextureCoords[0]) {
                vertices[i].uv = {
                    aiMesh->mTextureCoords[0][i].x,
                    aiMesh->mTextureCoords[0][i].y
                };
            }
        }

        // Indices
        indices.resize(aiMesh->mNumFaces * 3);
        for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
            aiFace face = aiMesh->mFaces[i];
            for (unsigned j = 0; j < face.mNumIndices; j++) {
                indices[i*3 + j] = face.mIndices[j];
            }
        }

        // Get texture path from material
        if (aiMesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                texturePath = texPath.C_Str();
            }
        }
    }
}
