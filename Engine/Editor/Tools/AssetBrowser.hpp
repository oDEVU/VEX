/**
 * @file   AssetBrowser.hpp
 * @brief  Class responsible for displaying and navigating project assets within an ImGUI window.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include <imgui.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace vex {

    /// @brief Structure holding texture IDs for various asset types displayed in the browser.
    struct BrowserIcons {
        ImTextureID folder = 0;
        ImTextureID file = 0;
        ImTextureID mesh = 0;
        ImTextureID texture = 0;
        ImTextureID scene = 0;
        ImTextureID text = 0;
        ImTextureID ui = 0;
    };

    /// @brief Class responsible for displaying and navigating project assets within an ImGUI window.
    class AssetBrowser {
    public:
    /**
             * @brief Constructs the AssetBrowser.
             * @param const std::string& rootPath - The root directory of the project assets.
             */
        AssetBrowser(const std::string& rootPath);

        /**
                 * @brief Draws the Asset Browser UI and handles interaction.
                 * @param const BrowserIcons& icons - The set of icons to use for drawing assets.
                 * @return std::string - The path of the asset that was double-clicked/selected for action, or empty string otherwise.
                 */
        std::string Draw(const BrowserIcons& icons);

        /**
                 * @brief Determines the type of a JSON asset (e.g., scene, material, etc.).
                 * @param const std::string path - The file path to check.
                 * @return int - An integer code representing the asset type.
                 */
        int GetJSONAssetType(const std::string path);

        /**
                 * @brief Extracts the file extension from a path.
                 * @param const std::string path - The file path.
                 * @return std::string - The file extension (e.g., ".png", ".scene").
                 */
        std::string GetExtension(const std::string path);

        /// @brief Sets the size for asset thumbnails in the browser.
                /// @param float size - The new thumbnail size in pixels.
        void setThumbnailSize(float size) { m_thumbnailSize = size; }

        /// @brief Sets the padding between asset icons in the browser.
                /// @param float padding - The new padding value.
        void setPadding(float padding) { m_padding = padding; }

        /// @brief Gets the currently viewed directory path.
                /// @return std::string - The current path as a string.
        std::string getCurrentPath() const { return m_currentPath.string(); }

    private:
        std::filesystem::path m_rootPath;
        std::filesystem::path m_currentPath;

        float m_thumbnailSize = 64.0f;
        float m_padding = 16.0f;

        std::filesystem::path m_copiedPath;
        bool m_isCut = false;

        bool m_renaming = false;
        std::filesystem::path m_renamePath;
        char m_renameBuffer[256] = "";

        /**
                 * @brief Gets the appropriate icon texture ID for a given directory entry.
                 * @param const std::filesystem::directory_entry& entry - The file system entry.
                 * @param const BrowserIcons& icons - The collection of available icons.
                 * @return ImTextureID - The texture ID of the selected icon.
                 */
        ImTextureID GetIcon(const std::filesystem::directory_entry& entry, const BrowserIcons& icons);
    };
}
