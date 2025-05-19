#include "mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <stdexcept>

namespace vex {
    void MeshData::loadFromFile(const std::string& path) {
        // Temporary debug code
        SDL_Log("Using Assimp version: %d.%d.%d",
               aiGetVersionMajor(),
               aiGetVersionMinor(),
               aiGetVersionRevision());

        SDL_Log("Creating assimp importer...");
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs);

        // Enhanced error checking
        if (!scene) {
            throw std::runtime_error("Assimp failed to load file: " + std::string(importer.GetErrorString()));
        }
        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            throw std::runtime_error("Assimp scene incomplete: " + std::string(importer.GetErrorString()));
        }
        if (!scene->mRootNode) {
            throw std::runtime_error("Assimp scene has no root node");
        }

        // Validate mesh array before access
        if (!scene->mMeshes) {
            throw std::runtime_error("Assimp scene has invalid mesh array");
        }
        if (scene->mNumMeshes == 0) {
            throw std::runtime_error("Model contains no meshes");
        }

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw std::runtime_error("Failed to load model: " + std::string(importer.GetErrorString()));
        }


        submeshes.clear();
        submeshes.resize(scene->mNumMeshes);
        for (unsigned m = 0; m < scene->mNumMeshes; m++) {

            SDL_Log("Processing mesh %i...", m);
            // Process first mesh only for simplicity
            aiMesh* aiMesh = scene->mMeshes[m];
            Submesh& submesh = submeshes[m];

            // Vertices
            SDL_Log("Loading verticies...");
            submesh.vertices.resize(aiMesh->mNumVertices);
            for (unsigned i = 0; i < aiMesh->mNumVertices; i++) {
                submesh.vertices[i].position = {
                    aiMesh->mVertices[i].x,
                    aiMesh->mVertices[i].y,
                    aiMesh->mVertices[i].z
                };

                submesh.vertices[i].normal = {
                    aiMesh->mNormals[i].x,
                    aiMesh->mNormals[i].y,
                    aiMesh->mNormals[i].z
                };

                if (aiMesh->mTextureCoords[0]) {
                    submesh.vertices[i].uv = {
                        aiMesh->mTextureCoords[0][i].x,
                        aiMesh->mTextureCoords[0][i].y
                    };
                }else{
                    SDL_Log("Invalid texure UVs...");
                    submesh.vertices[i].uv = glm::vec2(-1.0f);
                }
            }

            // Indices
            SDL_Log("Loading indices...");
            submesh.indices.resize(aiMesh->mNumFaces * 3);
            for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
                aiFace face = aiMesh->mFaces[i];
                for (unsigned j = 0; j < face.mNumIndices; j++) {
                    submesh.indices[i*3 + j] = face.mIndices[j];
                }
            }

            // Get texture path from material
            SDL_Log("Getting texture path...");
            if (aiMesh->mMaterialIndex >= 0) {
                aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    submesh.texturePath = texPath.C_Str();
                }
            }
        }
    }
}
