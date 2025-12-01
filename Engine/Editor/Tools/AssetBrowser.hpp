#pragma once
#include <imgui.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace vex {

    struct BrowserIcons {
        ImTextureID folder = 0;
        ImTextureID file = 0;
        ImTextureID mesh = 0;
        ImTextureID texture = 0;
        ImTextureID scene = 0;
        ImTextureID text = 0;
        ImTextureID ui = 0;
    };

    class AssetBrowser {
    public:
        AssetBrowser(const std::string& rootPath);

        std::string Draw(const BrowserIcons& icons);
        int GetJSONAssetType(const std::string path);
        std::string GetExtension(const std::string path);

    private:
        std::filesystem::path m_rootPath;
        std::filesystem::path m_currentPath;

        float m_thumbnailSize = 64.0f;
        float m_padding = 16.0f;

        ImTextureID GetIcon(const std::filesystem::directory_entry& entry, const BrowserIcons& icons);
    };
}
