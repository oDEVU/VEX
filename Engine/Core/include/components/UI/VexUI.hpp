/**
 *  @file   VexUI.hpp
 *  @brief  This file defines vex ui class, very basic ui system
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#ifndef VK_NO_PROTOTYPES
    #define VK_NO_PROTOTYPES 1
#endif
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <yoga/Yoga.h>
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include <volk.h>

#include "../../../src/components/backends/vulkan/context.hpp"
#include "../../../src/components/backends/vulkan/Resources.hpp"
#include "../../../src/components/VirtualFileSystem.hpp"

#include "../../../thirdparty/stb/stb_truetype.h"

namespace vex {

class VexUI;

/// @brief Font atlas structure
struct FontAtlas {
    stbtt_fontinfo info{};
    std::vector<stbtt_bakedchar> cdata;
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    uint32_t texIdx = 0;
    int width = 0, height = 0;
    float ascent = 0.f;
    float descent = 0.f;
    float scale = 0.f;
    float bakedSize = 0.f;
};

/// @brief Basic UI styling component
struct UIStyle {
    glm::vec4 color{1,1,1,1};
    glm::vec4 bgColor{-1,-1,-1,-1};
    glm::vec4 borderColor = {0,0,0,0};
    std::string font;
    float fontSize = 16.f;
    float borderWidth = 0.f;
};

/// @brief Allows for text aligment.
enum class TextAlign { Left, Center, Right };

/// @brief Quad component needed for rendering.
struct UIQuad {
    uint32_t vertexOffset;
    float texIndex;  // -1 = solid color
};

/// @brief Enum specifying possible types of widgets.
enum class WidgetType { Container, Label, Image, Button };

/// @brief Struct defining widget component, it has a lot of redundancy and probably will need cleanup before expanding on a feature.
struct Widget {
    WidgetType type = WidgetType::Container;
    std::string id;
    std::string text;
    std::string image;
    glm::vec2 size{0,0};
    float rotation = 0.f;
    UIStyle style;
    YGNodeRef yoga = nullptr;
    std::vector<Widget*> children;
    std::function<void()> onClick = nullptr;
    VexUI* ui = nullptr;
    TextAlign textAlign = TextAlign::Left;

    ~Widget();
    void applyJson(const nlohmann::json& j);
};

/// @brief Class defining VexUI, it initializes the UI system, loads, renders, converts ui data, essentially managing ui from file to reneering.
/// @todo make it into a UI_Component, so it renders like meshes and there can be multiple instances of ui. Also add z-index for sorting. Thank you later me.
class VexUI {
public:
    /// @brief Constructor for VexUI.
    /// @param VulkanContext& ctx - Vulkan context.
    /// @param VirtualFileSystem* vfs - Virtual file system.
    /// @param VulkanResources* res -Vulkan resources.
    VexUI(VulkanContext& ctx, VirtualFileSystem* vfs, VulkanResources* res);
    ~VexUI();

    /// @brief Initialize the UI system, components needed later on.
    bool init();
    /// @brief Load UI data from a JSON file.
    /// @param const std::string& path - Path to the JSON file.
    /// @details Example usage:
    /// @code
    /// m_vexUI->load("Assets/ui/example.json");
    /// @endcode
    void load(const std::string& path);
    /// @brief Render the UI. It is called by main rendering function.
    /// @param VkCommandBuffer cmd - Command buffer.
    /// @param VkPipeline pipeline - Pipeline.
    /// @param VkPipelineLayout pipelineLayout - Pipeline layout.
    /// @param int currentFrame - Current frame.
    void render(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipelineLayout, int currentFrame);
    /// @brief Process mouse and keyboard events.
    /// @param const SDL_Event& ev - SDL event.
    void processEvent(const SDL_Event& ev);

    /// @brief Set text of a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @param const std::string& txt - Text to set.
    /// @details Example usage:
    /// @code
    /// m_vexUI->setText("score", "FPS: " + std::to_string(fps));
    /// @endcode
    void setText(const std::string& id, const std::string& txt);
    /// @brief Set on-click callback of a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @param std::function<void()> cb - Callback function.
    /// @details Example usage:
    /// @code
    /// m_vexUI->setOnClick("pause", []() {
    ///     log("Pause button clicked");
    /// });
    /// @endcode
    void setOnClick(const std::string& id, std::function<void()> cb);

    /// @brief Get a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @return Widget& - Reference to the UI element.
    /// @details Example usage:
    /// @code
    /// Widget& scoreWidget = m_vexUI->getWidget("score");
    /// @endcode
    Widget* getWidget(const std::string& id) {
        if (Widget* w = findById(m_root, id)) return w;

        log("Widget not found");
        return nullptr;
    }

    /// @brief Get a UI element's style.
    /// @param const std::string& id - ID of the UI element.
    /// @return UIStyle& - Reference to the UI element's style.
    /// @details Example usage:
    /// @code
    /// UIStyle& scoreStyle = m_vexUI->getStyle("score");
    /// @endcode
    UIStyle* getStyle(const std::string& id) {
        if (Widget* w = findById(m_root, id)) return &w->style;
        log("Widget not found");
        return nullptr;
    }

    /// @brief Set a UI element's style.
    /// @param const std::string& id - ID of the UI element.
    /// @param const UIStyle& style - New style for the UI element.
    /// @details Example usage:
    /// @code
    /// UIStyle& scoreStyle = m_vexUI->getStyle("score");
    /// scoreStyle.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    /// m_vexUI->setStyle("score", scoreStyle);
    /// @endcode
    void setStyle(const std::string& id, const UIStyle& style) {
        if (initialized) {
            if (Widget* w = findById(m_root, id)) w->style = style;
            log("Widget not found");
        }else{
            pendingSetters.push_back([this, id, style]() {
                if (Widget* w = findById(m_root, id)) {
                    w->style = style;
                } else {
                    printf("Widget not found: %s", id.c_str());
                }
            });
        }
    }

    /// @brief Set rotation of a widget in degrees.
    void setRotation(const std::string& id, float degrees);

    /// @brief Move a widget (sets Left/Top yoga properties).
    /// Works best with position: absolute, or relative offsets.
    void setPosition(const std::string& id, float x, float y);

    /// @brief Resize a widget.
    void setSize(const std::string& id, float w, float h);

    /// @brief Change the image of an image widget.
    void setImage(const std::string& id, const std::string& path);

    /// @brief Change font properties.
    void setFont(const std::string& id, const std::string& fontPath, float fontSize);

    /// @brief Set text/tint color.
    void setColor(const std::string& id, glm::vec4 color);

    /// @brief Set background color.
    void setBackgroundColor(const std::string& id, glm::vec4 color);

    /// @brief Set border properties.
    void setBorder(const std::string& id, float width, glm::vec4 color);

    /// @brief Check if the UI system is initialized.
    /// @return bool - True if initialized, false otherwise.
    /// @details Example usage:
    /// @code
    /// if (m_vexUI->isInitialized()) {
    ///     // UI system is ready for use
    /// }
    /// @endcode
    bool isInitialized() {
        return initialized;
    }

    /// @brief Get the current z-index of the UI system.
    /// @return int - Current z-index.
    /// @details Example usage:
    /// @code
    /// int zIndex = m_vexUI->getZIndex();
    /// @endcode
    int getZIndex() {
        return zIndex;
    }

    /// @brief Set the z-index of the UI system.
    /// @param zIndex - New z-index value.
    /// @details Example usage:
    /// @code
    /// m_vexUI->setZIndex(10);
    /// @endcode
    void setZIndex(int zIndex) {
        if(initialized){
            this->zIndex = zIndex;
        }else{
            pendingSetters.push_back([this, zIndex]() {
                this->zIndex = zIndex;
            });
        }
    }
    /// @brief Update the UI system.
    /// @details This function should not be called directly. Its called every frame by the engine right before rendering to update pending operations.
    /// @code
    /// m_vexUI->update();
    /// @endcode
    void update() {
        if (initialized) {
            if (loadPending) {
                loadPending = false;
                load(loadPath);
            }
            for (auto& setter : pendingSetters) {
                setter();
            }
            pendingSetters.clear();
        }

    }

private:
    VulkanContext& m_ctx;
    VirtualFileSystem* m_vfs;
    VulkanResources* m_res;

    Widget* m_root = nullptr;
    bool initialized = false;
    int zIndex = 0;

    std::vector<std::function<void()>> pendingSetters;

    bool loadPending = false;
    std::string loadPath = "";

    std::unordered_map<std::string, FontAtlas> m_fontAtlases;

    VkBuffer m_vb = VK_NULL_HANDLE;
    VmaAllocation m_vbAlloc = VK_NULL_HANDLE;
    size_t m_vbSize = 0;
    VkSampler m_uiSampler = VK_NULL_HANDLE;

    void loadFonts(Widget* w);
    void loadImages(Widget* w);
    void layout(glm::uvec2 res);
    void batch(Widget* w, std::vector<float>& verts, glm::vec2 parentOffset = {0.f, 0.f});
    void uploadVerts(const std::vector<float>& verts);
    Widget* parseNode(const nlohmann::json& j);
    void freeTree(Widget* w);
    Widget* findWidgetAt(Widget* w, glm::vec2 pos, glm::vec2 parentOffset = {0.f, 0.f});
    Widget* findById(Widget* w, const std::string& id);
    YGSize calculateTextSize(Widget* w, float maxWidth = FLT_MAX);
    static YGSize measureTextNode(const YGNode* node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode);
    void safeUpdate(const std::string& id, std::function<void(Widget*)> action);

};
}
