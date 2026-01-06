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

    /// @brief Initializes the virtual file system.
    /// @details Sets the base path and checks for the existence of a packed asset file (`Assets/assets.vpk`).
    /// If the VPK exists, it attempts to load the header and file table.
    /// If the VPK is missing (or in Debug builds), it falls back to loose file loading (`m_use_packed_assets = false`).
    /// @param const std::string& base_path - The root directory path (usually the executable directory) to initialize the VFS from.
    /// @return bool - true if initialization (VPK load or fallback setup) is successful, false otherwise.
    bool initialize(const std::string& base_path);

    /// @brief Loads a file into memory from the specified path.
    /// @details
    /// - **Packed Mode**: Locates the file entry in the loaded VPK, seeks to the data offset, and reads `entry->data_size` bytes.
    /// - **Loose Mode**: Reads the file from disk using `std::ifstream`, resolving the path relative to the base directory.
    /// @param const std::string& virtual_path - The relative path or unique ID of the file to load.
    /// @return std::unique_ptr<FileData> - Unique pointer to the struct containing the raw data vector and size, or nullptr if not found.
    std::unique_ptr<FileData> load_file(const std::string& virtual_path);

    /// @brief Opens a file stream for reading from a specified path.
    /// @details
    /// - **Packed Mode**: Reads the specific file chunk from the VPK into a memory buffer (protected by `stream_mutex`) and returns a custom `VPKStream` wrapped around that buffer.
    /// - **Loose Mode**: returns a standard `std::ifstream` opened in binary mode.
    /// @param const std::string& virtual_path - The relative path to the file.
    /// @return std::unique_ptr<std::istream> - Unique pointer to the input stream, or nullptr if the file cannot be opened.
    std::unique_ptr<std::istream> open_file_stream(const std::string& virtual_path);

    /// @brief Checks if a file exists in the currently active file system mode.
    /// @details
    /// - **Packed Mode**: Searches the `m_loaded_vpk->file_names` vector for the cleaned path.
    /// - **Loose Mode**: Uses `std::filesystem::exists` on the physical disk path.
    /// @param const std::string& virtual_path - The path to check.
    /// @return bool - true if the file exists, false otherwise.
    bool file_exists(const std::string& virtual_path);

    /// @brief Gets the size of a file in bytes.
    /// @details Returns `entry->data_size` for packed files or `fs::file_size` for loose files.
    /// @param const std::string& virtual_path - The path of the file.
    /// @return size_t - Size of the file in bytes, or 0 if not found.
    size_t get_file_size(const std::string& virtual_path);

    /// @brief Lists all files contained within a specific directory.
    /// @details
    /// - **Packed Mode**: Iterates VPK file names and filters by the provided directory prefix.
    /// - **Loose Mode**: Uses `fs::recursive_directory_iterator` to traverse the physical `Assets/` directory.
    /// @param const std::string& virtual_dir - The directory path to filter by (default is root).
    /// @return std::vector<std::string> - A vector of relative file paths found.
    std::vector<std::string> list_files(const std::string& virtual_dir = "");

    /// @brief Resolves a relative path to a normalized, cleaner format.
    /// @details Uses `std::filesystem::lexically_normal()` to resolve `..` and `.` segments, and ensures consistent separators via `clean_path`.
    /// @param const std::string& base_path - The context path (e.g., the path of the file requesting the resolve).
    /// @param const std::string& relative_path - The path relative to `base_path`.
    /// @return std::string - The resolved, cleaned virtual path.
    std::string resolve_relative_path(const std::string& base_path, const std::string& relative_path);

    /// @brief Gets the base path of the virtual file system.
    /// @return std::string - base path.
    std::string get_base_path() const { return m_base_path; }
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
