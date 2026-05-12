#include "components/VirtualFileSystem.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include "components/ErrorUtils.hpp"
#include <zstd.h>

namespace vex {

std::string VirtualFileSystem::s_vpak_key = "FALLBACK_VPAK_KEY_0000";

void VirtualFileSystem::SetVpakKey(const std::string& key) {
    s_vpak_key = key;
}

std::string VirtualFileSystem::GetVpakKey() {
    return s_vpak_key;
}

VirtualFileSystem::VirtualFileSystem()
    : m_use_packed_assets(false) {
}

VirtualFileSystem::~VirtualFileSystem() {
}

bool VirtualFileSystem::initialize(const std::string& base_path) {
    m_base_path = base_path;

#if DEBUG
    m_use_packed_assets = false;
    return true;
#else
    m_use_packed_assets = true;
    std::string vpk_path = base_path + "/Assets/assets.vpk";

    if (fs::exists(vpk_path)) {
        return load_vpk_file(vpk_path);
    } else {
        m_use_packed_assets = false;
        log(LogLevel::WARNING, "VPK file [ path: %s ] not found, falling back to loose files", vpk_path.c_str());
        return true;
    }
#endif
}

bool VirtualFileSystem::load_vpk_file(const std::string& vpk_path) {
    if (s_vpak_key == "FALLBACK_VPAK_KEY_0000") {
        log(LogLevel::ERROR, "VirtualFileSystem: Using fallback VPAK key - assets may not decrypt correctly!");
    }

    m_loaded_vpk = std::make_unique<LoadedVPK>();
    m_loaded_vpk->file_path = vpk_path;

    m_loaded_vpk->file_stream.open(vpk_path, std::ios::binary);
    if (!m_loaded_vpk->file_stream) {
        log(LogLevel::ERROR, "Failed to open VPK file: %s", vpk_path.c_str());
        throw_error("Failed to open VPK file.");
        m_loaded_vpk.reset();
        return false;
    }

    m_loaded_vpk->file_stream.read(
        reinterpret_cast<char*>(&m_loaded_vpk->header),
        sizeof(VPKHeader)
    );

    if (std::strncmp(m_loaded_vpk->header.magic, "VPAK", 4) != 0) {
        throw_error("Invalid VPK file: bad magic");
        m_loaded_vpk.reset();
        return false;
    }

    if (m_loaded_vpk->header.version != 3) {
        log(LogLevel::WARNING, "VPK version mismatch: expected 3, got %u", m_loaded_vpk->header.version);
    }

    m_loaded_vpk->entries.resize(m_loaded_vpk->header.file_count);
    m_loaded_vpk->file_stream.seekg(sizeof(VPKHeader));
    m_loaded_vpk->file_stream.read(
        reinterpret_cast<char*>(m_loaded_vpk->entries.data()),
        m_loaded_vpk->header.file_count * sizeof(VPKFileEntry)
    );

    m_loaded_vpk->file_stream.seekg(m_loaded_vpk->header.names_offset);
    for (uint32_t i = 0; i < m_loaded_vpk->header.file_count; ++i) {
        std::string name;
        std::getline(m_loaded_vpk->file_stream, name, '\0');
        m_loaded_vpk->file_names.push_back(name);
    }

    std::string key = GetVpakKey();

    std::vector<uint8_t> compressed_buffer(m_loaded_vpk->header.solid_compressed_size);

    {
        std::lock_guard<std::mutex> lock(stream_mutex);
        m_loaded_vpk->file_stream.seekg(m_loaded_vpk->header.data_offset);
        m_loaded_vpk->file_stream.read(
            reinterpret_cast<char*>(compressed_buffer.data()),
            m_loaded_vpk->header.solid_compressed_size
        );
    }

    /*for (size_t j = 0; j < compressed_buffer.size(); ++j) {
        compressed_buffer[j] ^= key[j % key.length()];
        }*/

    size_t key_len = key.length();
    size_t key_idx = 0;
    for (size_t j = 0; j < compressed_buffer.size(); ++j) {
        compressed_buffer[j] ^= key[key_idx];
        key_idx++;
        if (key_idx == key_len) {
            key_idx = 0;
        }
    }

    m_loaded_vpk->solid_data.resize(m_loaded_vpk->header.solid_uncompressed_size);
    std::memset(m_loaded_vpk->solid_data.data(), 0, m_loaded_vpk->header.solid_uncompressed_size);

    size_t result = ZSTD_decompress(
        m_loaded_vpk->solid_data.data(), m_loaded_vpk->header.solid_uncompressed_size,
        compressed_buffer.data(), m_loaded_vpk->header.solid_compressed_size
    );

    if (ZSTD_isError(result)) {
        log(LogLevel::ERROR, "Zstd decompression failed: %s", ZSTD_getErrorName(result));
        m_loaded_vpk.reset();
        return false;
    }

    log("Loaded VPK with %u files", m_loaded_vpk->header.file_count);

    return true;
}

const VirtualFileSystem::VPKFileEntry* VirtualFileSystem::find_file_entry(const std::string& virtual_path) {
    if (!m_loaded_vpk) return nullptr;

    std::string cleanVirtualPath = clean_path(virtual_path);
    auto it = std::find(
        m_loaded_vpk->file_names.begin(),
        m_loaded_vpk->file_names.end(),
        cleanVirtualPath
    );

    if (it == m_loaded_vpk->file_names.end()) {
        return nullptr;
    }

    size_t index = std::distance(m_loaded_vpk->file_names.begin(), it);
    return &m_loaded_vpk->entries[index];
}

std::unique_ptr<VirtualFileSystem::FileData> VirtualFileSystem::load_file(const std::string& virtual_path) {
    std::string cleanVirtualPath = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(cleanVirtualPath);
        if (!entry) {
            return nullptr;
        }

        auto fileData = std::make_unique<FileData>();
        fileData->size = entry->uncompressed_size;
        fileData->data.resize(entry->uncompressed_size);

        std::memcpy(fileData->data.data(),
                    m_loaded_vpk->solid_data.data() + entry->data_offset,
                    entry->uncompressed_size);

        return fileData;
    } else {
        std::string fullPath;

    #ifdef _WIN32
        fullPath = cleanVirtualPath;
    #else
        fullPath = "/" + cleanVirtualPath;
    #endif

        if (!fs::exists(fullPath)) {
            return nullptr;
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        auto fileData = std::make_unique<FileData>();
        fileData->data.resize(size);
        fileData->size = size;

        file.read(reinterpret_cast<char*>(fileData->data.data()), size);
        return fileData;
    }
}

std::unique_ptr<std::istream> VirtualFileSystem::open_file_stream(const std::string& virtual_path) {
    std::string cleanVirtualPath = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(cleanVirtualPath);
        if (!entry) {
            return nullptr;
        }

        return std::make_unique<VPKStream>(
            reinterpret_cast<const char*>(m_loaded_vpk->solid_data.data() + entry->data_offset),
            entry->uncompressed_size
        );
    } else {
        std::string fullPath;

    #ifdef _WIN32
        fullPath = cleanVirtualPath;
    #else
        fullPath = "/" + cleanVirtualPath;
    #endif
        if (!fs::exists(fullPath)) {
            return nullptr;
        }
        return std::make_unique<std::ifstream>(fullPath, std::ios::binary);
    }
}

bool VirtualFileSystem::file_exists(const std::string& virtual_path) {
    std::string cleanVirtualPath = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        return find_file_entry(cleanVirtualPath) != nullptr;
    } else {
        std::string fullPath;

    #ifdef _WIN32
        fullPath = cleanVirtualPath;
    #else
        fullPath = "/" + cleanVirtualPath;
    #endif
        return fs::exists(fullPath);
    }
}

size_t VirtualFileSystem::get_file_size(const std::string& virtual_path) {
    std::string cleanVirtualPath = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(cleanVirtualPath);
        if (!entry) {
            return 0;
        }
        return entry->uncompressed_size;
    } else {
        std::string fullPath;

    #ifdef _WIN32
        fullPath = cleanVirtualPath;
    #else
        fullPath = "/" + cleanVirtualPath;
    #endif
        if (fs::exists(fullPath)) {
            return fs::file_size(fullPath);
        }
        return 0;
    }
}

std::vector<std::string> VirtualFileSystem::list_files(const std::string& virtual_dir) {
    std::vector<std::string> result;
    std::string cleanDir = clean_path(virtual_dir);

    if (m_use_packed_assets && m_loaded_vpk) {
        for (const auto& name : m_loaded_vpk->file_names) {
            if (cleanDir.empty() || name.find(cleanDir) == 0) {
                result.push_back(name);
            }
        }
    } else {
        std::string fullDir = m_base_path + "/Assets/" + cleanDir;
        if (fs::exists(fullDir) && fs::is_directory(fullDir)) {
            for (const auto& entry : fs::recursive_directory_iterator(fullDir)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = fs::relative(entry.path(), m_base_path + "/Assets").generic_string();
                    result.push_back(relativePath);
                }
            }
        }
    }

    return result;
}

std::string VirtualFileSystem::clean_path(const std::string& path) {
    std::string result = path;

    std::replace(result.begin(), result.end(), '\\', '/');

    if (result.find("./") == 0) {
        result = result.substr(2);
    }

    if (!result.empty() && result[0] == '/') {
        result = result.substr(1);
    }

    if (result.find("Assets/") == 0 && m_use_packed_assets) {
        result = result.substr(7);
    }

    return result;
}

std::string VirtualFileSystem::resolve_relative_path(const std::string& base_path, const std::string& relative_path) {
    std::filesystem::path base(base_path);
    std::filesystem::path relative(relative_path);

    if (relative.is_absolute()) {
        return clean_path(relative.string());
    }

    base.remove_filename();
    std::filesystem::path resolved = base / relative;
    return clean_path(resolved.lexically_normal().string());
}

}
