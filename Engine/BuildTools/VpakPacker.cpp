#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <zstd.h>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct VPKHeader {
    char magic[4] = {'V','P','A','K'};
    uint32_t version = 3;
    uint32_t file_count;
    uint32_t names_offset;
    uint32_t data_offset;
    uint32_t solid_compressed_size;
    uint32_t solid_uncompressed_size;
};

struct VPKFileEntry {
    uint32_t name_offset;
    uint32_t data_offset;
    uint32_t uncompressed_size;
};
#pragma pack(pop)

class VPKPacker {
private:
    std::vector<std::string> file_names;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> file_data;
    int compression_level = 0;

public:
    bool pack_directory(const std::string& input_dir, const std::string& output_file, const std::string& encryption_key, int compression_level_arg) {
        compression_level = ((compression_level_arg * 3) + 1);
        compression_level = std::clamp(compression_level, 1, 22);

        if (!collect_files(input_dir, input_dir)) {
            return false;
        }

        uint32_t names_offset = sizeof(VPKHeader) + file_names.size() * sizeof(VPKFileEntry);
        uint32_t total_names_size = 0;
        for (const auto& name : file_names) {
            total_names_size += static_cast<uint32_t>(name.size()) + 1;
        }

        uint32_t data_offset = names_offset + total_names_size;

        size_t total_uncompressed_size = 0;
        for (const auto& [path, data] : file_data) {
            total_uncompressed_size += data.size();
        }

        std::vector<uint8_t> solid_uncompressed_buffer;
        solid_uncompressed_buffer.reserve(total_uncompressed_size);

        std::vector<VPKFileEntry> entries;
        uint32_t current_solid_offset = 0;

        for (size_t i = 0; i < file_data.size(); ++i) {
            VPKFileEntry entry;
            entry.data_offset = current_solid_offset;
            entry.uncompressed_size = static_cast<uint32_t>(file_data[i].second.size());
            entries.push_back(entry);

            solid_uncompressed_buffer.insert(
                solid_uncompressed_buffer.end(),
                file_data[i].second.begin(),
                file_data[i].second.end()
            );
            current_solid_offset += entry.uncompressed_size;
        }

        size_t max_compressed_size = ZSTD_compressBound(total_uncompressed_size);
        std::vector<uint8_t> solid_compressed_buffer(max_compressed_size);

        size_t compressed_size = ZSTD_compress(
            solid_compressed_buffer.data(), max_compressed_size,
            solid_uncompressed_buffer.data(), total_uncompressed_size,
            compression_level
        );

        if (ZSTD_isError(compressed_size)) {
            std::cerr << "Zstd compression failed: " << ZSTD_getErrorName(compressed_size) << std::endl;
            return false;
        }
        solid_compressed_buffer.resize(compressed_size);

        for (size_t i = 0; i < solid_compressed_buffer.size(); ++i) {
            solid_compressed_buffer[i] ^= encryption_key[i % encryption_key.length()];
        }

        VPKHeader header;
        header.file_count = static_cast<uint32_t>(file_names.size());
        header.names_offset = names_offset;
        header.data_offset = data_offset;
        header.solid_compressed_size = static_cast<uint32_t>(compressed_size);
        header.solid_uncompressed_size = static_cast<uint32_t>(total_uncompressed_size);

        std::ofstream out(output_file, std::ios::binary);
        if (!out) {
            std::cerr << "Failed to create output file: " << output_file << std::endl;
            return false;
        }

        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        uint32_t current_name_offset = 0;
        for (size_t i = 0; i < file_names.size(); ++i) {
            entries[i].name_offset = current_name_offset;
            out.write(reinterpret_cast<const char*>(&entries[i]), sizeof(VPKFileEntry));
            current_name_offset += static_cast<uint32_t>(file_names[i].size()) + 1;
        }

        for (const auto& name : file_names) {
            out.write(name.c_str(), name.size() + 1);
        }

        out.write(reinterpret_cast<const char*>(solid_compressed_buffer.data()), solid_compressed_buffer.size());

        std::cout << "Packed " << file_names.size() << " files into " << output_file << " (Zstd Level " << compression_level << ")" << std::endl;
        std::cout << "Uncompressed: " << total_uncompressed_size / 1024 << " KB -> Compressed: " << compressed_size / 1024 << " KB" << std::endl;
        return true;
    }

private:
    bool collect_files(const std::string& root_dir, const std::string& current_dir) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(current_dir)) {
                if (entry.is_regular_file()) {
                    std::string absolute_path = entry.path().string();
                    std::string relative_path = fs::relative(absolute_path, root_dir).generic_string();

                    std::ifstream file(absolute_path, std::ios::binary);
                    if (!file) {
                        std::cerr << "Failed to read file: " << absolute_path << std::endl;
                        return false;
                    }

                    file.seekg(0, std::ios::end);
                    size_t size = file.tellg();
                    file.seekg(0, std::ios::beg);

                    std::vector<uint8_t> data(size);
                    file.read(reinterpret_cast<char*>(data.data()), size);

                    file_names.push_back(relative_path);
                    file_data.emplace_back(absolute_path, std::move(data));
                }
            }
            return true;
        } catch (const fs::filesystem_error& ex) {
            std::cerr << "Filesystem error: " << ex.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <input_directory> <output_file.vpk> <encryption_key> <compression_level>" << std::endl;
        return 1;
    }

    std::string input_dir = argv[1];
    std::string output_file = argv[2];
    std::string key = argv[3];
    int compression_level = std::atoi(argv[4]);

    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Input directory does not exist: " << input_dir << std::endl;
        return 1;
    }

    VPKPacker packer;
    if (!packer.pack_directory(input_dir, output_file, key, compression_level)) {
        return 1;
    }

    return 0;
}
