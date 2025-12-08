#include "components/Mesh.hpp"
#include "components/errorUtils.hpp"
#include "components/VirtualFileSystem.hpp" // Add this include

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
    std::vector<char> buffer_;
    size_t position_;

public:
    VPKAssimpStream(const char* data, size_t size)
        : buffer_(data, data + size), position_(0) {}

    ~VPKAssimpStream() override = default;

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        size_t bytes_to_read = pSize * pCount;
        size_t bytes_available = buffer_.size() - position_;

        if (bytes_to_read > bytes_available) {
            bytes_to_read = bytes_available;
        }

        if (bytes_to_read > 0) {
            memcpy(pvBuffer, buffer_.data() + position_, bytes_to_read);
            position_ += bytes_to_read;
        }

        return bytes_to_read / pSize; // Return number of items read
    }

    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
        return 0; // Read-only
    }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        size_t new_position = position_;

        switch (pOrigin) {
            case aiOrigin_SET: new_position = pOffset; break;
            case aiOrigin_CUR: new_position = position_ + pOffset; break;
            case aiOrigin_END: new_position = buffer_.size() + pOffset; break;
            default: return aiReturn_FAILURE;
        }

        if (new_position > buffer_.size()) {
            return aiReturn_FAILURE;
        }

        position_ = new_position;
        return aiReturn_SUCCESS;
    }

    size_t Tell() const override {
        return position_;
    }

    size_t FileSize() const override {
        return buffer_.size();
    }

    void Flush() override {}
};

// Custom Assimp IO system for VPK files
class VPKAssimpIOSystem : public Assimp::IOSystem {
private:
    VirtualFileSystem* vfs_;
    std::string base_path_;
    std::string base_dir_;

public:
    VPKAssimpIOSystem(VirtualFileSystem* vfs, const std::string& base_path)
        : vfs_(vfs), base_path_(base_path) {
        // Extract directory from base path
        std::filesystem::path path_obj(base_path);
        base_dir_ = path_obj.parent_path().string();
        if (!base_dir_.empty() && base_dir_.back() != '/') {
            base_dir_ += '/';
        }
    }

    ~VPKAssimpIOSystem() override = default;

    bool Exists(const char* pFile) const override {
        std::string file_path(pFile);

        log("Assimp checking if file exists: '%s'", file_path.c_str());

        // First try the path as-is
        if (vfs_->file_exists(file_path)) {
            log("File exists as-is: '%s'", file_path.c_str());
            return true;
        }

        // Try relative to the base directory
        std::string relative_path = base_dir_ + file_path;
        if (vfs_->file_exists(relative_path)) {
            log("File exists relative to base: '%s'", relative_path.c_str());
            return true;
        }

        // Try just the filename (for cases like "scene.bin")
        std::filesystem::path path_obj(file_path);
        std::string just_filename = path_obj.filename().string();
        if (vfs_->file_exists(just_filename)) {
            log("File exists as filename only: '%s'", just_filename.c_str());
            return true;
        }

        log(LogLevel::WARNING, "File not found: '%s'", file_path.c_str());
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

        std::string file_path(pFile);
        log("Assimp trying to open: '%s'", file_path.c_str());

        std::string final_path;

        // Try different path resolutions in order:

        // 1. Try as-is
        if (vfs_->file_exists(file_path)) {
            final_path = file_path;
            log("Opening file as-is: '%s'", final_path.c_str());
        }
        // 2. Try relative to base directory
        else if (vfs_->file_exists(base_dir_ + file_path)) {
            final_path = base_dir_ + file_path;
            log("Opening file relative to base: '%s'", final_path.c_str());
        }
        // 3. Try just the filename
        else {
            std::filesystem::path path_obj(file_path);
            std::string just_filename = path_obj.filename().string();
            if (vfs_->file_exists(just_filename)) {
                final_path = just_filename;
                log("Opening file as filename only: '%s'", final_path.c_str());
            } else {
                log(LogLevel::ERROR, "Failed to find file: '%s'", file_path.c_str());
                return nullptr;
            }
        }

        // Read the file data into memory first
        auto file_data = vfs_->load_file(final_path);
        if (!file_data) {
            log("Failed to load file data: '%s'", final_path.c_str());
            return nullptr;
        }

        log("Successfully loaded file: '%s' (%zu bytes)", final_path.c_str(), file_data->size);

        // Create stream from memory buffer
        return new VPKAssimpStream(reinterpret_cast<const char*>(file_data->data.data()), file_data->size);
    }

    void Close(Assimp::IOStream* pFile) override {
        delete pFile;
    }
};

    void MeshData::loadFromFile(const std::string& path, VirtualFileSystem* vfs) {
        //static VirtualFileSystem* vfs = nullptr; // set this

        // Temporary debug code
        log("Using Assimp version: %d.%d.%d",
               aiGetVersionMajor(),
               aiGetVersionMinor(),
               aiGetVersionRevision());

        log("Creating assimp importer...");

        auto importerPtr = std::make_unique<Assimp::Importer>();
        Assimp::Importer& importer = *importerPtr;

        std::string realPath = path;

        #if NDEBUG
                // Set up custom IO system if using VFS
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
            if (!fileData) {
                throw_error("Failed to load file from VFS: " + realPath);
            }

            log("Loading from VFS memory buffer, size: %zu", fileData->size);

            // Get file extension for format hint
            std::string extension = std::filesystem::path(realPath).extension().string();
            const char* formatHint = extension.c_str();

            // Use ReadFileFromMemory instead of ReadFile
            scene = importer.ReadFileFromMemory(
                fileData->data.data(),
                fileData->size,
                aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs,
                formatHint);
#else
        scene = importer.ReadFile(realPath,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs);

#endif
        if (!scene) {
            handle_exception(std::runtime_error("Assimp failed to load file: " + std::string(importer.GetErrorString())));
            return;
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
                } else {
                    submesh.vertices[i].uv = glm::vec2(-100000.f);
                }
            }

            // Indices
            log("Loading indices...");
            submesh.indices.reserve(aiMesh->mNumFaces * 3);
            for (unsigned i = 0; i < aiMesh->mNumFaces; i++) {
                aiFace face = aiMesh->mFaces[i];
                for (unsigned j = 0; j < face.mNumIndices; j++) {
                    submesh.indices.push_back(face.mIndices[j]);
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

        // Clean up custom IO system
        if (vfs) {
            importer.SetIOHandler(nullptr);
        }
    }
}
