#include "AssetBrowser.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>

#include "components/errorUtils.hpp"
#include "components/pathUtils.hpp"

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
        std::string actionFile = "";

        if (m_currentPath != m_rootPath) {
            if (ImGui::Button(" < Back ")) {
                m_currentPath = m_currentPath.parent_path();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
                    std::string sourcePathStr = (const char*)payload->Data;
                    std::filesystem::path sourcePath(sourcePathStr);
                    std::filesystem::path parentDir = m_currentPath.parent_path();
                    std::filesystem::path destPath = parentDir / sourcePath.filename();

                    try {
                        if (sourcePath != destPath) {
                            std::filesystem::rename(sourcePath, destPath);
                        }
                    } catch (const std::filesystem::filesystem_error& e) {
                        vex::log("Error moving file up: %s", e.what());
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();
        }

        std::filesystem::path assetDir = GetAssetDir();
        std::filesystem::path relative = m_currentPath.lexically_relative(assetDir);

        ImGui::Text("Path: %s", relative.string().c_str());
        ImGui::Separator();

        if (ImGui::BeginPopupContextWindow("AssetBrowserBgContext", ImGuiMouseButton_Right | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Paste", nullptr, false, !m_copiedPath.empty())) {
                    if (std::filesystem::exists(m_copiedPath)) {
                        try {
                            std::filesystem::path dest = m_currentPath / m_copiedPath.filename();
                            int i = 1;
                            std::string stem = m_copiedPath.stem().string();
                            std::string ext = m_copiedPath.extension().string();
                            while (std::filesystem::exists(dest)) {
                                dest = m_currentPath / (stem + " (" + std::to_string(i++) + ")" + ext);
                            }

                            std::filesystem::copy(m_copiedPath, dest, std::filesystem::copy_options::recursive);

                            if (m_isCut) {
                                std::filesystem::remove_all(m_copiedPath);
                                m_copiedPath.clear();
                                m_isCut = false;
                            }
                        } catch (const std::filesystem::filesystem_error& e) {
                            vex::handle_exception(e);
                        }
                    }
                }
                ImGui::EndPopup();
            }

        float cellSize = m_thumbnailSize + m_padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        if (ImGui::BeginTable("BrowserGrid", columnCount)) {
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
                vex::handle_exception(e);
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

                ImGui::ImageButton("##Icon", iconID, { m_thumbnailSize, m_thumbnailSize });

                if (ImGui::BeginDragDropSource()) {
                    std::string fullPath = entry.path().string();

                    ImGui::SetDragDropPayload("ASSET_ITEM", fullPath.c_str(), fullPath.size() + 1);

                    ImGui::Text("Move %s", entry.path().filename().string().c_str());
                    ImGui::Image(iconID, { m_thumbnailSize/2, m_thumbnailSize/2 });

                    ImGui::EndDragDropSource();
                }

                if (entry.is_directory()) {
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
                            std::string sourcePathStr = (const char*)payload->Data;
                            std::filesystem::path sourcePath(sourcePathStr);
                            std::filesystem::path targetFolder = entry.path();
                            std::filesystem::path destPath = targetFolder / sourcePath.filename();

                            try {
                                bool isSubFolder = (targetFolder.string().find(sourcePath.string()) == 0);

                                if (sourcePath != destPath && !isSubFolder) {
                                    std::filesystem::rename(sourcePath, destPath);
                                }
                            } catch (const std::filesystem::filesystem_error& e) {
                                vex::log("Error moving file: %s", e.what());
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        if (entry.is_directory()) {
                            m_currentPath /= entry.path().filename();
                        } else {
                            actionFile = entry.path().string();
                        }
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        selectedFile = entry.path().string();
                    }
                }

                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Open")) {
                        if (entry.is_directory()) m_currentPath /= entry.path().filename();
                        else actionFile = entry.path().string();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy")) {
                        m_copiedPath = entry.path();
                        m_isCut = false;
                    }
                    if (ImGui::MenuItem("Cut")) {
                         m_copiedPath = entry.path();
                         m_isCut = true;
                    }
                    if (ImGui::MenuItem("Duplicate")) {
                        try {
                             std::filesystem::path src = entry.path();
                             std::filesystem::path dest = src;
                             int i = 1;
                             while(std::filesystem::exists(dest)) {
                                 dest = src.parent_path() / (src.stem().string() + "_Copy" + std::to_string(i++) + src.extension().string());
                             }
                             std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive);
                        } catch(const std::filesystem::filesystem_error& e) {
                            vex::handle_exception(e);
                        }
                    }
                    if (ImGui::MenuItem("Rename")) {
                        m_renaming = true;
                        m_renamePath = entry.path();
                        strncpy(m_renameBuffer, filename.c_str(), sizeof(m_renameBuffer));
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete")) {
                        try { std::filesystem::remove_all(entry.path()); } catch(...) {}
                    }
                    ImGui::EndPopup();
                }

                if (m_renaming && m_renamePath == entry.path()) {
                    ImGui::SetNextItemWidth(cellSize);
                    if (ImGui::InputText("##Rename", m_renameBuffer, sizeof(m_renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                        std::filesystem::path newPath = entry.path().parent_path() / m_renameBuffer;
                        try {
                            std::filesystem::rename(entry.path(), newPath);
                        } catch (const std::filesystem::filesystem_error& e) {
                            vex::handle_exception(e);
                        }
                        m_renaming = false;
                    }
                    if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                        m_renaming = false;
                    }
                }
                else {
                    float columnWidth = ImGui::GetContentRegionAvail().x;
                    float textWidth = ImGui::CalcTextSize(filename.c_str()).x;

                    if (textWidth < (columnWidth * 1.1f)) {
                        float textOffset = (columnWidth - textWidth) * 0.5f;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
                        ImGui::Text("%s", filename.c_str());
                    } else {
                        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + columnWidth);
                        ImGui::TextWrapped("%s", filename.c_str());
                        ImGui::PopTextWrapPos();
                    }
                }

                ImGui::PopID();
            };

            for (const auto& entry : folders) { renderEntry(entry); }
            for (const auto& entry : files)   { renderEntry(entry); }

            ImGui::EndTable();
        }

        return actionFile;
    }
}
