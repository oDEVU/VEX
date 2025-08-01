#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>

namespace vex {
    void MeshData::loadFromFile(const std::string& path) {
        // Temporary debug code
        log("Using Assimp version: %d.%d.%d",
               aiGetVersionMajor(),
               aiGetVersionMinor(),
               aiGetVersionRevision());

        log("Creating assimp importer...");
        Assimp::Importer importer;

        std::string realPath = path;//GetAssetPath(path);

        if (!std::filesystem::exists(realPath)){
            throw_error("File: [" + realPath + "] doesnt exists");
        }

        const aiScene* scene = importer.ReadFile(realPath,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs);

        if (!scene) {
            throw_error("Assimp failed to load file: " + std::string(importer.GetErrorString()));
        }
        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            throw_error("Assimp scene incomplete: " + std::string(importer.GetErrorString()));
        }
        if (!scene->mRootNode) {
            throw_error("Assimp scene has no root node");
        }
        if (!scene->mMeshes) {
            throw_error("Assimp scene has invalid mesh array");
        }
        if (scene->mNumMeshes == 0) {
            throw_error("Model contains no meshes");
        }
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            throw_error("Failed to load model: " + std::string(importer.GetErrorString()));
        }


        submeshes.clear();
        submeshes.resize(scene->mNumMeshes);
        for (unsigned m = 0; m < scene->mNumMeshes; m++) {

            log("Processing mesh %i...", m);
            aiMesh* aiMesh = scene->mMeshes[m];
            Submesh& submesh = submeshes[m];

            // Vertices
            log("Loading verticies...");
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
                    //log("Invalid texure UVs...");
                    submesh.vertices[i].uv = glm::vec2(-1.0f);
                }
            }

            // Indices
            log("Loading indices...");
            submesh.indices.resize(aiMesh->mNumFaces * 3);
            for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
                aiFace face = aiMesh->mFaces[i];
                for (unsigned j = 0; j < face.mNumIndices; j++) {
                    submesh.indices[i*3 + j] = face.mIndices[j];
                }
            }

            // Textures
            log("Getting texture path...");

            std::filesystem::path meshPath(realPath);
            meshPath.remove_filename();

            if (aiMesh->mMaterialIndex >= 0) {
                aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    submesh.texturePath = meshPath.string() + texPath.C_Str();
                }
            }
        }
    }
}
