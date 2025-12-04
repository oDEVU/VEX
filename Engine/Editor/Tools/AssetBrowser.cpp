#include "AssetBrowser.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>

namespace vex {

    AssetBrowser::AssetBrowser(const std::string& rootPath)
        : m_rootPath(rootPath), m_currentPath(rootPath) {
    }

    ImTextureID AssetBrowser::GetIcon(const std::filesystem::directory_entry& entry, const BrowserIcons& icons) {
        if (entry.is_directory()) return icons.folder;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") return icons.texture;
        if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") return icons.mesh;
        if (ext == ".txt" || ext == ".md") return icons.text;
        if (ext == ".json"){
            int type = GetJSONAssetType(entry.path().string());
            if (type == 1) return icons.scene;
            if (type == 2) return icons.ui;
            return icons.text;
        }

        return icons.file;
    }

    std::string AssetBrowser::GetExtension(const std::string path){
        std::filesystem::path p(path);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    int AssetBrowser::GetJSONAssetType(const std::string path){
        std::ifstream f(path);
        nlohmann::json data = nlohmann::json::parse(f);

        if (data.contains("environment") || data.contains("objects")){
            return 1;
        } else if (data.contains("root")) {
            return 2;
        }
        return 0;
    }

    std::string AssetBrowser::Draw(const BrowserIcons& icons) {
        std::string selectedFile = "";

        if (m_currentPath != m_rootPath) {
            if (ImGui::Button(" < Back ")) {
                m_currentPath = m_currentPath.parent_path();
            }
            ImGui::SameLine();
        }
        ImGui::Text("Path: %s", m_currentPath.string().c_str());
        ImGui::Separator();

        float cellSize = m_thumbnailSize + m_padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        if (ImGui::BeginTable("BrowserGrid", columnCount)) {// 1. Collect and Separate Entries
            std::vector<std::filesystem::directory_entry> folders;
            std::vector<std::filesystem::directory_entry> files;

            try {
                for (const auto& entry : std::filesystem::directory_iterator(m_currentPath)) {
                    if (entry.is_directory()) {
                        folders.push_back(entry);
                    } else {
                        files.push_back(entry);
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                //fuck it, idk what to do
            }

            auto sortAlpha = [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                return a.path().filename().string() < b.path().filename().string();
            };
            std::sort(folders.begin(), folders.end(), sortAlpha);
            std::sort(files.begin(), files.end(), sortAlpha);

            auto renderEntry = [&](const std::filesystem::directory_entry& entry) {
                ImGui::TableNextColumn();
                std::string filename = entry.path().filename().string();
                ImGui::PushID(filename.c_str());

                float columnWidth = ImGui::GetContentRegionAvail().x;

                ImTextureID iconID = GetIcon(entry, icons);
                if (iconID == 0) iconID = icons.file;

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                if (ImGui::ImageButton("##Icon", iconID, { m_thumbnailSize, m_thumbnailSize })) {
                    if (entry.is_directory()) {
                        m_currentPath /= entry.path().filename();
                    }
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (!entry.is_directory()) {
                        selectedFile = entry.path().string();
                    }
                }

                ImGui::PopStyleColor();

                float textWidth = ImGui::CalcTextSize(filename.c_str()).x;

                if (textWidth < (columnWidth * 1.1f)) {
                    float textOffset = (columnWidth - textWidth) * 0.5f;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
                    ImGui::Text("%s", filename.c_str());
                }
                else {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + columnWidth);
                    ImGui::TextWrapped("%s", filename.c_str());
                    ImGui::PopTextWrapPos();
                }

                ImGui::PopID();
            };

            for (const auto& entry : folders) { renderEntry(entry); }
            for (const auto& entry : files)   { renderEntry(entry); }

            ImGui::EndTable();
        }

        return selectedFile;
    }
}
