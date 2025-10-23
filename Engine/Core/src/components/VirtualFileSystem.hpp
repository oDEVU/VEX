/**
 *  @file   VirtualFileSystem.hpp
 *  @brief  This file defines VirtualFileSystem and VPKStream classes.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <mutex>

namespace fs = std::filesystem;

namespace vex {

/// @brief This class provides abstraction of file system needed for loading packed and unpacked assets.
class VirtualFileSystem {
public:
    /// @brief Simple struct needed to represent loaded file with data and size.
    struct FileData {
        /// @brief raw file data
        std::vector<uint8_t> data;
        /// @brief size of the file data in bytes.
        size_t size;
    };

    /// @brief Constructor for VirtualFileSystem.
    VirtualFileSystem();
    ~VirtualFileSystem();

    /// @brief Initializes the virtual file system checks if file exists and is proper format.
    /// @param const std::string& base_path
    /// @return bool - true if initialization is successful, false otherwise.
    bool initialize(const std::string& base_path);

    /// @brief Loads file from a specified path. For packed assets path is unique file id.
    /// @param const std::string& virtual_path
    /// @return std::unique_ptr<FileData> - unique pointer to the loaded file data.
    std::unique_ptr<FileData> load_file(const std::string& virtual_path);

    /// @brief Opens a file stream from a specified path. For packed assets path is unique file id.
    /// @param const std::string& virtual_path
    /// @return std::unique_ptr<std::istream> - unique pointer to the opened file stream.
    std::unique_ptr<std::istream> open_file_stream(const std::string& virtual_path);

    /// @brief Checks if a file exists in the virtual file system.
    /// @param const std::string& virtual_path
    /// @return bool - true if the file exists, false otherwise.
    bool file_exists(const std::string& virtual_path);

    /// @brief Gets the size of a file in the virtual file system.
    /// @param const std::string& virtual_path
    /// @return size_t - size of the file in bytes.
    size_t get_file_size(const std::string& virtual_path);

    /// @brief Lists all files in a directory in the virtual file system.
    /// @param const std::string& virtual_dir - directory path to list files from.
    /// @return std::vector<std::string> - vector of file paths.
    std::vector<std::string> list_files(const std::string& virtual_dir = "");

    /// @brief Gets the base path of the virtual file system.
    /// @return std::string - base path.
    std::string get_base_path() const { return m_base_path; }

    /// @brief Resolves a relative path to an absolute path.
    /// @param const std::string& base_path - base path to resolve from.
    /// @param const std::string& relative_path - relative path to resolve.
    /// @return std::string - absolute path.
    std::string resolve_relative_path(const std::string& base_path, const std::string& relative_path);

private:
    /// @brief struct defining vpak file header data, used to validate file integrity.
    struct VPKHeader {
        char magic[4];
        uint32_t version;
        uint32_t file_count;
        uint32_t names_offset;
        uint32_t data_offset;
    };

    /// @brief struct defining virtual file "header", used to identify files in the virtual file system.
    struct VPKFileEntry {
        uint32_t name_offset;
        uint32_t data_offset;
        uint32_t data_size;
        uint32_t uncompressed_size;
    };

    /// @brief struct holding data of loaded vpk file.
    struct LoadedVPK {
        VPKHeader header;
        std::vector<VPKFileEntry> entries;
        std::vector<std::string> file_names;
        std::ifstream file_stream;
        std::string file_path;
    };

    /// @brief class implementing custom file stream functionality for virtual file system. Inherits from std::istream.
    class VPKStream : public std::istream {
    private:
        class VPKStreamBuf : public std::streambuf {
        private:
            std::vector<char> buffer;

        public:
            /// @brief Method to initialize the buffer with data.
            VPKStreamBuf(const char* data, size_t size) {
                buffer.resize(size);
                std::memcpy(buffer.data(), data, size);
                setg(buffer.data(), buffer.data(), buffer.data() + size);
            }
        };

        VPKStreamBuf buf;

    public:
        /// @brief Constructor to initialize the buffer with data.
        VPKStream(const char* data, size_t size)
            : std::istream(&buf), buf(data, size) {}
    };

    std::unique_ptr<LoadedVPK> m_loaded_vpk;
    std::string m_base_path;
    bool m_use_packed_assets;

    std::mutex stream_mutex;

    /// @brief Method to load a VPK file.
    /// @param const std::string& vpk_path - path to vpk file
    /// @return true if the file was loaded successfully, false otherwise
    bool load_vpk_file(const std::string& vpk_path);

    /// @brief cleans path from unwanted string eg "Assets/"
    std::string clean_path(const std::string& path);

    /// @brief Method to find a file entry by its virtual path.
    const VPKFileEntry* find_file_entry(const std::string& virtual_path);
};

}
