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

class VirtualFileSystem {
public:
    struct FileData {
        std::vector<uint8_t> data;
        size_t size;
    };

    VirtualFileSystem();
    ~VirtualFileSystem();

    bool initialize(const std::string& base_path);
    std::unique_ptr<FileData> load_file(const std::string& virtual_path);
    std::unique_ptr<std::istream> open_file_stream(const std::string& virtual_path);
    bool file_exists(const std::string& virtual_path);
    size_t get_file_size(const std::string& virtual_path);
    std::vector<std::string> list_files(const std::string& virtual_dir = "");
    std::string get_base_path() const { return m_base_path; }
    std::string resolve_relative_path(const std::string& base_path, const std::string& relative_path);

private:
    struct VPKHeader {
        char magic[4];
        uint32_t version;
        uint32_t file_count;
        uint32_t names_offset;
        uint32_t data_offset;
    };

    struct VPKFileEntry {
        uint32_t name_offset;
        uint32_t data_offset;
        uint32_t data_size;
        uint32_t uncompressed_size;
    };

    struct LoadedVPK {
        VPKHeader header;
        std::vector<VPKFileEntry> entries;
        std::vector<std::string> file_names;
        std::ifstream file_stream;
        std::string file_path;
    };

    class VPKStream : public std::istream {
    private:
        class VPKStreamBuf : public std::streambuf {
        private:
            std::vector<char> buffer;

        public:
            VPKStreamBuf(const char* data, size_t size) {
                buffer.resize(size);
                std::memcpy(buffer.data(), data, size);
                setg(buffer.data(), buffer.data(), buffer.data() + size);
            }
        };

        VPKStreamBuf buf;

    public:
        VPKStream(const char* data, size_t size)
            : std::istream(&buf), buf(data, size) {}
    };

    std::unique_ptr<LoadedVPK> m_loaded_vpk;
    std::string m_base_path;
    bool m_use_packed_assets;

    std::mutex stream_mutex;

    bool load_vpk_file(const std::string& vpk_path);
    std::string clean_path(const std::string& path);
    const VPKFileEntry* find_file_entry(const std::string& virtual_path);
};

}
