#include "VirtualFileSystem.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include "components/errorUtils.hpp"

namespace vex {

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
        // Fall back to loose files even in release if VPK doesn't exist
        m_use_packed_assets = false;
        //std::cout << "VPK file [ path: " << vpk_path << "] not found, falling back to loose files" << std::endl;
        log("VPK file [ path: %s ] not found, falling back to loose files", vpk_path.c_str());
        return true;
    }
#endif
}

bool VirtualFileSystem::load_vpk_file(const std::string& vpk_path) {
    m_loaded_vpk = std::make_unique<LoadedVPK>();
    m_loaded_vpk->file_path = vpk_path;

    m_loaded_vpk->file_stream.open(vpk_path, std::ios::binary);
    if (!m_loaded_vpk->file_stream) {
        //std::cerr << "Failed to open VPK file: " << vpk_path << std::endl;
        log("Failed to open VPK file: %s", vpk_path.c_str());
        throw_error("Failed to open VPK file.");
        m_loaded_vpk.reset();
        return false;
    }

    m_loaded_vpk->file_stream.read(
        reinterpret_cast<char*>(&m_loaded_vpk->header),
        sizeof(VPKHeader)
    );

    if (std::strncmp(m_loaded_vpk->header.magic, "VPAK", 4) != 0) {
        //std::cerr << "Invalid VPK file: bad magic" << std::endl;
        throw_error("Invalid VPK file: bad magic");
        m_loaded_vpk.reset();
        return false;
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

    //std::cout << "Loaded VPK with " << m_loaded_vpk->header.file_count << " files" << std::endl;
    log("Loaded VPK with %u files", m_loaded_vpk->header.file_count);

    // DEBUG: List all files in VPK
    log("Files in VPK:");
    for (const auto& name : m_loaded_vpk->file_names) {
        log("File: %s", name.c_str());
    }

    return true;
}

const VirtualFileSystem::VPKFileEntry* VirtualFileSystem::find_file_entry(const std::string& virtual_path) {
    if (!m_loaded_vpk) return nullptr;

    std::string clean_virtual_path = clean_path(virtual_path);
    auto it = std::find(
        m_loaded_vpk->file_names.begin(),
        m_loaded_vpk->file_names.end(),
        clean_virtual_path
    );

    if (it == m_loaded_vpk->file_names.end()) {
        return nullptr;
    }

    size_t index = std::distance(m_loaded_vpk->file_names.begin(), it);
    return &m_loaded_vpk->entries[index];
}

std::unique_ptr<VirtualFileSystem::FileData> VirtualFileSystem::load_file(const std::string& virtual_path) {
    std::string clean_virtual_path = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(clean_virtual_path);
        if (!entry) {
            return nullptr;
        }

        // Read data from VPK
        auto file_data = std::make_unique<FileData>();
        file_data->data.resize(entry->data_size);
        file_data->size = entry->data_size;

        m_loaded_vpk->file_stream.seekg(m_loaded_vpk->header.data_offset + entry->data_offset);
        m_loaded_vpk->file_stream.read(
            reinterpret_cast<char*>(file_data->data.data()),
            entry->data_size
        );

        return file_data;
    } else {
        std::string full_path = "/" +clean_virtual_path;

        if (!fs::exists(full_path)) {
            return nullptr;
        }

        std::ifstream file(full_path, std::ios::binary);
        if (!file) {
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        auto file_data = std::make_unique<FileData>();
        file_data->data.resize(size);
        file_data->size = size;

        file.read(reinterpret_cast<char*>(file_data->data.data()), size);
        return file_data;
    }
}

std::unique_ptr<std::istream> VirtualFileSystem::open_file_stream(const std::string& virtual_path) {
    std::string clean_virtual_path = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(clean_virtual_path);
        if (!entry) {
            return nullptr;
        }

        // Read the file data into memory
        std::vector<char> buffer(entry->data_size);

        // Use the main stream but protect with seek/read
        std::lock_guard<std::mutex> lock(stream_mutex); // Add a mutex to your class
        m_loaded_vpk->file_stream.seekg(m_loaded_vpk->header.data_offset + entry->data_offset);
        m_loaded_vpk->file_stream.read(buffer.data(), entry->data_size);

        if (m_loaded_vpk->file_stream.gcount() != entry->data_size) {
            return nullptr;
        }

        return std::make_unique<VPKStream>(buffer.data(), buffer.size());
    } else {
        std::string full_path = "/" +clean_virtual_path;
        if (!fs::exists(full_path)) {
            return nullptr;
        }
        return std::make_unique<std::ifstream>(full_path, std::ios::binary);
    }
}

bool VirtualFileSystem::file_exists(const std::string& virtual_path) {
    std::string clean_virtual_path = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        return find_file_entry(clean_virtual_path) != nullptr;
    } else {
        std::string full_path = "/" +clean_virtual_path;
        log(full_path.c_str());
        return fs::exists(full_path);
    }
}

size_t VirtualFileSystem::get_file_size(const std::string& virtual_path) {
    std::string clean_virtual_path = clean_path(virtual_path);

    if (m_use_packed_assets && m_loaded_vpk) {
        const auto* entry = find_file_entry(clean_virtual_path);
        return entry ? entry->data_size : 0;
    } else {
        std::string full_path = "/" +clean_virtual_path;
        if (fs::exists(full_path)) {
            return fs::file_size(full_path);
        }
        return 0;
    }
}

std::vector<std::string> VirtualFileSystem::list_files(const std::string& virtual_dir) {
    std::vector<std::string> result;
    std::string clean_dir = clean_path(virtual_dir);

    if (m_use_packed_assets && m_loaded_vpk) {
        for (const auto& name : m_loaded_vpk->file_names) {
            if (clean_dir.empty() || name.find(clean_dir) == 0) {
                result.push_back(name);
            }
        }
    } else {
        std::string full_dir = m_base_path + "/Assets/" + clean_dir;
        if (fs::exists(full_dir) && fs::is_directory(full_dir)) {
            for (const auto& entry : fs::recursive_directory_iterator(full_dir)) {
                if (entry.is_regular_file()) {
                    std::string relative_path = fs::relative(entry.path(), m_base_path + "/Assets").generic_string();
                    result.push_back(relative_path);
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

    if (result.find("Assets/") == 0) {
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
