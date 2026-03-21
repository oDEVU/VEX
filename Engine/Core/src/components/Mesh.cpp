#include "components/Mesh.hpp"
#include "components/ErrorUtils.hpp"
#include "components/VirtualFileSystem.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

namespace vex {

// Custom Assimp IO stream for VPK files
class VPKAssimpStream : public Assimp::IOStream {
private:
    std::vector<char> m_buffer;
    size_t m_position;

public:
    VPKAssimpStream(const char* data, size_t size)
        : m_buffer(data, data + size), m_position(0) {}

    ~VPKAssimpStream() override = default;

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        size_t bytesToRead = pSize * pCount;
        size_t bytesAvailable = m_buffer.size() - m_position;

        if (bytesToRead > bytesAvailable) {
            bytesToRead = bytesAvailable;
        }

        if (bytesToRead > 0) {
            memcpy(pvBuffer, m_buffer.data() + m_position, bytesToRead);
            m_position += bytesToRead;
        }

        return bytesToRead / pSize; // Return number of items read
    }

    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
        return 0; // Read-only
    }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        size_t newPosition = m_position;

        switch (pOrigin) {
            case aiOrigin_SET: newPosition = pOffset; break;
            case aiOrigin_CUR: newPosition = m_position + pOffset; break;
            case aiOrigin_END: newPosition = m_buffer.size() + pOffset; break;
            default: return aiReturn_FAILURE;
        }

        if (newPosition > m_buffer.size()) {
            return aiReturn_FAILURE;
        }

        m_position = newPosition;
        return aiReturn_SUCCESS;
    }

    size_t Tell() const override {
        return m_position;
    }

    size_t FileSize() const override {
        return m_buffer.size();
    }

    void Flush() override {}
};

// Custom Assimp IO system for VPK files
class VPKAssimpIOSystem : public Assimp::IOSystem {
private:
    VirtualFileSystem* m_vfs;
    std::string m_basePath;
    std::string m_baseDir;

public:
    VPKAssimpIOSystem(VirtualFileSystem* vfs, const std::string& base_path)
        : m_vfs(vfs), m_basePath(base_path) {
        // Extract directory from base path
        std::filesystem::path pathObj(base_path);
        m_baseDir = pathObj.parent_path().string();
        if (!m_baseDir.empty() && m_baseDir.back() != '/') {
            m_baseDir += '/';
        }
    }

    ~VPKAssimpIOSystem() override = default;

    bool Exists(const char* pFile) const override {
        std::string filePath(pFile);

        log("Assimp checking if file exists: '%s'", filePath.c_str());

        // First try the path as-is
        if (m_vfs->file_exists(filePath)) {
            log("File exists as-is: '%s'", filePath.c_str());
            return true;
        }

        // Try relative to the base directory
        std::string relativePath = m_baseDir + filePath;
        if (m_vfs->file_exists(relativePath)) {
            log("File exists relative to base: '%s'", relativePath.c_str());
            return true;
        }

        // Try just the filename (for cases like "scene.bin")
        std::filesystem::path pathObj(filePath);
        std::string justFilename = pathObj.filename().string();
        if (m_vfs->file_exists(justFilename)) {
            log("File exists as filename only: '%s'", justFilename.c_str());
            return true;
        }

        log(LogLevel::WARNING, "File not found: '%s'", filePath.c_str());
        return false;
    }

    char getOsSeparator() const override {
        return '/';
    }

    Assimp::IOStream* Open(const char* pFile, const char* pMode) override {
        // Only support read mode
        if (std::strstr(pMode, "r") == nullptr) {
            return nullptr;
        }

        std::string filePath(pFile);
        log("Assimp trying to open: '%s'", filePath.c_str());

        std::string finalPath;

        // Try different path resolutions in order:

        // 1. Try as-is
        if (m_vfs->file_exists(filePath)) {
            finalPath = filePath;
            log("Opening file as-is: '%s'", finalPath.c_str());
        }
        // 2. Try relative to base directory
        else if (m_vfs->file_exists(m_baseDir + filePath)) {
            finalPath = m_baseDir + filePath;
            log("Opening file relative to base: '%s'", finalPath.c_str());
        }
        // 3. Try just the filename
        else {
            std::filesystem::path pathObj(filePath);
            std::string justFilename = pathObj.filename().string();
            if (m_vfs->file_exists(justFilename)) {
                finalPath = justFilename;
                log("Opening file as filename only: '%s'", finalPath.c_str());
            } else {
                log(LogLevel::ERROR, "Failed to find file: '%s'", filePath.c_str());
                return nullptr;
            }
        }

        // Read the file data into memory first
        auto fileData = m_vfs->load_file(finalPath);
        if (!fileData) {
            log("Failed to load file data: '%s'", finalPath.c_str());
            return nullptr;
        }

        log("Successfully loaded file: '%s' (%zu bytes)", finalPath.c_str(), fileData->size);

        // Create stream from memory buffer
        return new VPKAssimpStream(reinterpret_cast<const char*>(fileData->data.data()), fileData->size);
    }

    void Close(Assimp::IOStream* pFile) override {
        delete pFile;
    }
};

void MeshData::processScene(const aiScene* scene, const std::string& textureBaseDir) {
        if (scene->mNumMeshes == 0) {
            throw_error("Model contains no meshes");
        }

        submeshes.clear();
        submeshes.resize(scene->mNumMeshes);

        for (unsigned m = 0; m < scene->mNumMeshes; m++) {
            log("Processing mesh %i...", m);
            aiMesh* aiMesh = scene->mMeshes[m];
            Submesh& submesh = submeshes[m];

            submesh.vertices.resize(aiMesh->mNumVertices);
            for (unsigned i = 0; i < aiMesh->mNumVertices; i++) {
                submesh.vertices[i].position = {
                    aiMesh->mVertices[i].x,
                    aiMesh->mVertices[i].y,
                    aiMesh->mVertices[i].z
                };

                if(aiMesh->mNormals) {
                    submesh.vertices[i].normal = {
                        aiMesh->mNormals[i].x,
                        aiMesh->mNormals[i].y,
                        aiMesh->mNormals[i].z
                    };
                }

                if (aiMesh->mTextureCoords[0]) {
                    submesh.vertices[i].uv = {
                        aiMesh->mTextureCoords[0][i].x,
                        aiMesh->mTextureCoords[0][i].y
                    };
                } else {
                    submesh.vertices[i].uv = glm::vec2(-100000.f);
                }
            }

            submesh.indices.reserve(aiMesh->mNumFaces * 3);
            for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
                aiFace face = aiMesh->mFaces[i];
                for (unsigned j = 0; j < face.mNumIndices; j++) {
                    submesh.indices.push_back(face.mIndices[j]);
                }
            }

            if (aiMesh->mMaterialIndex >= 0) {
                aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    submesh.texturePath = textureBaseDir + texPath.C_Str();
                }
            }
        }
    }

    void MeshData::loadFromRawFile(const std::string& relativePath) {
        std::filesystem::path execDir = GetExecutableDir();
        std::filesystem::path fullPath = execDir / relativePath;
        std::string pathStr = fullPath.string();

        log("Loading raw mesh from: %s", pathStr.c_str());

        if (!std::filesystem::exists(fullPath)) {
            handle_exception(std::runtime_error("Raw file does not exist: " + pathStr));
            return;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(pathStr,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            handle_exception(std::runtime_error("Assimp failed to load raw file: " + std::string(importer.GetErrorString())));
            return;
        }

        std::filesystem::path meshFolder = fullPath;
        meshFolder.remove_filename();

        processScene(scene, meshFolder.string());

        meshPath = pathStr;
    }

    void MeshData::loadFromFile(const std::string& path, VirtualFileSystem* vfs) {
        log("Using Assimp version: %d.%d.%d", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());
        log("Creating assimp importer...");

        auto importerPtr = std::make_unique<Assimp::Importer>();
        Assimp::Importer& importer = *importerPtr;

        std::string realPath = path;

        #if NDEBUG
            if (vfs) {
                importer.SetIOHandler(new VPKAssimpIOSystem(vfs, realPath));
            }
        #endif

        if (!vfs->file_exists(realPath)){
            handle_exception(std::runtime_error("File: [" + realPath + "] doesnt exist"));
            return;
        }

        const aiScene* scene = nullptr;

        #if NDEBUG
            auto fileData = vfs->load_file(realPath);
            if (!fileData) throw_error("Failed to load file from VFS: " + realPath);

            log("Loading from VFS memory buffer, size: %zu", fileData->size);

            std::string extension = std::filesystem::path(realPath).extension().string();
            scene = importer.ReadFileFromMemory(
                fileData->data.data(),
                fileData->size,
                aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs,
                extension.c_str());
        #else
            scene = importer.ReadFile(realPath,
                aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
        #endif

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            handle_exception(std::runtime_error("Assimp failed to load file: " + std::string(importer.GetErrorString())));
            return;
        }

        std::filesystem::path meshFolderPath(realPath);
        meshFolderPath.remove_filename();

        processScene(scene, meshFolderPath.string());

        if (vfs) importer.SetIOHandler(nullptr);
        meshPath = realPath;
    }
}
