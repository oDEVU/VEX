//This file was generated in big part by AI (sorry :C)

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct VPKHeader {
    char magic[4] = {'V','P','A','K'};
    uint32_t version = 1;
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
#pragma pack(pop)

class VPKPacker {
private:
    std::vector<std::string> file_names;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> file_data;

public:
    bool pack_directory(const std::string& input_dir, const std::string& output_file) {
        // Collect all files
        if (!collect_files(input_dir, input_dir)) {
            return false;
        }

        // Calculate offsets
        uint32_t names_offset = sizeof(VPKHeader) + file_names.size() * sizeof(VPKFileEntry);
        uint32_t current_data_offset = 0;

        // Create header
        VPKHeader header;
        header.file_count = static_cast<uint32_t>(file_names.size());
        header.names_offset = names_offset;

        // Calculate total names size
        uint32_t total_names_size = 0;
        for (const auto& name : file_names) {
            total_names_size += static_cast<uint32_t>(name.size()) + 1; // +1 for null terminator
        }

        header.data_offset = names_offset + total_names_size;

        // Create file entries
        std::vector<VPKFileEntry> entries;
        for (const auto& [path, data] : file_data) {
            VPKFileEntry entry;
            // name_offset will be filled later
            entry.data_offset = current_data_offset;
            entry.data_size = static_cast<uint32_t>(data.size());
            entry.uncompressed_size = entry.data_size; // No compression for now
            entries.push_back(entry);
            current_data_offset += entry.data_size;
        }

        // Write to file
        std::ofstream out(output_file, std::ios::binary);
        if (!out) {
            std::cerr << "Failed to create output file: " << output_file << std::endl;
            return false;
        }

        // Write header
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write file entries
        for (const auto& entry : entries) {
            out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        }

        // Write file names
        uint32_t current_name_offset = 0;
        for (size_t i = 0; i < file_names.size(); ++i) {
            // Update name offset in the corresponding entry (we need to rewrite entries)
            VPKFileEntry entry = entries[i];
            entry.name_offset = current_name_offset;

            // Seek back and update the entry
            std::streampos current_pos = out.tellp();
            out.seekp(sizeof(VPKHeader) + i * sizeof(VPKFileEntry));
            out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
            out.seekp(current_pos);

            // Write the name
            out.write(file_names[i].c_str(), file_names[i].size() + 1);
            current_name_offset += static_cast<uint32_t>(file_names[i].size()) + 1;
        }

        // Write file data
        for (const auto& [path, data] : file_data) {
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        std::cout << "Packed " << file_names.size() << " files into " << output_file << std::endl;
        return true;
    }

private:
    bool collect_files(const std::string& root_dir, const std::string& current_dir) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(current_dir)) {
                if (entry.is_regular_file()) {
                    std::string absolute_path = entry.path().string();
                    std::string relative_path = fs::relative(absolute_path, root_dir).generic_string();

                    // Read file data
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
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_directory> <output_file.vpk>" << std::endl;
        return 1;
    }

    std::string input_dir = argv[1];
    std::string output_file = argv[2];

    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Input directory does not exist: " << input_dir << std::endl;
        return 1;
    }

    VPKPacker packer;
    if (!packer.pack_directory(input_dir, output_file)) {
        return 1;
    }

    return 0;
}
