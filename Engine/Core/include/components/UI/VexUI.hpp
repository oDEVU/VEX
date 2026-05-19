/**
 * @file   VexUI.hpp
 * @brief  This file defines vex ui class, very basic ui system
 * @author Eryk Roszkowski
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
#include "components/ECS/ECS.hpp"
#include <SDL3/SDL.h>
#include <volk.h>

#include "../../../src/components/backends/vulkan/context.hpp"
#include "../../../src/components/backends/vulkan/Resources.hpp"
#include "components/VirtualFileSystem.hpp"
#include "components/ResolutionManager.hpp"

#include "../../../thirdparty/stb/stb_truetype.h"

namespace vex {

class VexUI;

/// @brief Supported responsive UI unit types.
enum class UIUnitType { Auto, Px, Psp, Vw, Vh, Percent };

/// @brief Struct storing unit value logic natively.
struct UIUnitValue {
    float value = 0.f;
    UIUnitType type = UIUnitType::Psp;

    UIUnitValue() = default;
    UIUnitValue(float v) : value(v), type(UIUnitType::Psp) {}
    UIUnitValue(float v, UIUnitType t) : value(v), type(t) {}

    // Allows backward compatibility.
    UIUnitValue& operator=(float v) {
        value = v;
        return *this;
    }

    /// @brief Parses a json value into the unit struct.
    void parse(const nlohmann::json& jval);

    static UIUnitValue parseJson(const nlohmann::json& jval) {
        UIUnitValue v; v.parse(jval); return v;
    }

    /// @brief Converts the unit back to a json-compatible format (preserves strings like "px", "%").
    nlohmann::json toJson() const;

    /// @brief Dynamically calculates the raw pixel representation required for rendering.
    float getPixels(const VexUI* ui) const;
};

/// @brief 2D representation of UIUnitValue to replace glm::vec2 size safely.
struct UIUnitVec2 {
    UIUnitValue x{0.f, UIUnitType::Auto};
    UIUnitValue y{0.f, UIUnitType::Auto};
};

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
    UIUnitValue fontSize{16.f, UIUnitType::Psp};
    UIUnitValue borderWidth{0.f, UIUnitType::Psp};
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

    UIUnitVec2 size;
    float rotation = 0.f;
    UIStyle style;

    nlohmann::json nodeJson;

    YGNodeRef yoga = nullptr;
    std::vector<Widget*> children;
    Widget* parent = nullptr;
    std::function<void()> onClick = nullptr;
    std::function<void()> onFocusEnter = nullptr;
    std::function<void()> onFocusLost = nullptr;
    VexUI* ui = nullptr;
    TextAlign textAlign = TextAlign::Left;

    ~Widget();
    void applyLayout(VexUI* uiManager);
};

/// @brief Class defining VexUI, it initializes the UI system, loads, renders, converts ui data, essentially managing ui from file to reneering.
class VexUI {
public:
    /// @brief Constructor for VexUI.
    /// @param VulkanContext& ctx - Vulkan context.
    /// @param VirtualFileSystem* vfs - Virtual file system.
    /// @param VulkanResources* res -Vulkan resources.
    /// @param ResolutionManager* resMgr - Resolution manager.
    VexUI(VulkanContext& ctx, VirtualFileSystem* vfs, VulkanResources* res, ResolutionManager* resMgr);
    ~VexUI();

    /// @brief Initialize the UI system, components needed later on.
    bool init();
    /// @brief Load UI data from a JSON file.
    /// @param const std::string& path - Path to the JSON file.
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

    /// @brief Get the currently focused widget.
    /// @return Widget* - Pointer to focused widget, or nullptr if none focused.
    Widget* getFocusedWidget() const { return m_focusedWidget; }

    /// @brief Set text of a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @param const std::string& txt - Text to set.
    void setText(const std::string& id, const std::string& txt);

    /// @brief Set on-click callback of a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @param std::function<void()> cb - Callback function.
    void setOnClick(const std::string& id, std::function<void()> cb);

    /// @brief Get a UI element.
    /// @param const std::string& id - ID of the UI element.
    /// @return Widget* - Pointer to the UI element.
    Widget* getWidget(const std::string& id) {
        if (Widget* w = findById(m_root, id)) return w;
        log("Widget not found");
        return nullptr;
    }

    /// @brief Get a UI element's style.
    /// @param const std::string& id - ID of the UI element.
    /// @return UIStyle* - Pointer to the UI element's style.
    UIStyle* getStyle(const std::string& id) {
        if (Widget* w = findById(m_root, id)) return &w->style;
        log("Widget not found");
        return nullptr;
    }

    /// @brief Set a UI element's style.
    /// @param const std::string& id - ID of the UI element.
    /// @param const UIStyle& style - New style for the UI element.
    void setStyle(const std::string& id, const UIStyle& style) {
        if (initialized) {
            if (Widget* w = findById(m_root, id)) w->style = style;
        } else {
            pendingSetters.push_back([this, id, style]() {
                if (Widget* w = findById(m_root, id)) w->style = style;
            });
        }
    }

    /// @brief Set rotation of a widget in degrees.
    /// @param const std::string& id The id of the widget to rotate.
    /// @param float degrees The rotation angle in degrees.
    void setRotation(const std::string& id, float degrees);

    /// @brief Move a widget (sets Left/Top yoga properties). Works best with position: absolute, or relative offsets.
    /// @param const std::string& id The id of widget to Move
    /// @param UIUnitValue x The x-coordinate of the widget's new position.
    /// @param UIUnitValue y The y-coordinate of the widget's new position.
    void setPosition(const std::string& id, UIUnitValue x, UIUnitValue y);

    /// @brief Resize a widget.
    /// @param const std::string& id The id of the widget to resize.
    /// @param UIUnitValue w The new width of the widget.
    /// @param UIUnitValue h The new height of the widget.
    void setSize(const std::string& id, UIUnitValue w, UIUnitValue h);

    /// @brief Change the image of an image widget.
    /// @param const std::string& id The id of the widget to change the image of.
    /// @param const std::string& path The path to the new image.
    void setImage(const std::string& id, const std::string& path);

    /// @brief Change font properties.
    /// @param const std::string& id The id of the widget to change the font of.
    /// @param const std::string& fontPath The path to the new font.
    /// @param UIUnitValue fontSize The new font size.
    void setFont(const std::string& id, const std::string& fontPath, UIUnitValue fontSize);

    /// @brief Set text/tint color.
    /// @param const std::string& id The id of the widget to change the color of.
    /// @param glm::vec4 color The new color.
    void setColor(const std::string& id, glm::vec4 color);

    /// @brief Set background color.
    /// @param const std::string& id The id of the widget to change the background color of.
    /// @param glm::vec4 color The new background color.
    void setBackgroundColor(const std::string& id, glm::vec4 color);

    /// @brief Set border properties.
    /// @param const std::string& id The id of the widget to change the border of.
    /// @param UIUnitValue width The new border width.
    /// @param glm::vec4 color The new border color.
    void setBorder(const std::string& id, UIUnitValue width, glm::vec4 color);

    /// @brief Check if the UI system is initialized.
    /// @return bool - True if initialized, false otherwise.
    bool isInitialized() { return initialized; }

    /// @brief Get the current z-index of the UI system.
    /// @return int - Current z-index.
    int getZIndex() { return zIndex; }

    /// @brief Set the z-index of the UI system.
    /// @param zIndex - New z-index value.
    void setZIndex(int zIndex) {
        if(initialized) { this->zIndex = zIndex; }
        else { pendingSetters.push_back([this, zIndex]() { this->zIndex = zIndex; }); }
    }

    /// @brief Update the UI system.
    /// @details This function should not be called directly. Its called every frame by the engine right before rendering to update pending operations.
    void update(float deltaTime = 0.016f) {
        if (initialized) {
            if (m_gamepadNavigationCooldown > 0.0f) m_gamepadNavigationCooldown -= deltaTime;
            if (loadPending) {
                loadPending = false;
                load(loadPath);
            }
            for (auto& setter : pendingSetters) setter();
            pendingSetters.clear();
        }
    }

    float getPspMultiplier() const;
    glm::uvec2 getRenderResolution() const { return m_ctx.currentRenderResolution; }

    void applyYogaDimension(YGNodeRef yoga, const UIUnitValue& val, void(*setPx)(YGNodeRef, float), void(*setPct)(YGNodeRef, float), void(*setAuto)(YGNodeRef) = nullptr) const;
    void applyYogaMargin(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const;
    void applyYogaPadding(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const;
    void applyYogaPosition(YGNodeRef yoga, YGEdge edge, const UIUnitValue& val) const;

private:
    VulkanContext& m_ctx;
    VirtualFileSystem* m_vfs;
    VulkanResources* m_res;
    ResolutionManager* m_resMgr;

    Widget* m_root = nullptr;
    bool initialized = false;
    int zIndex = 0;

    glm::uvec2 m_lastRenderRes{0, 0};
    float m_lastPspMult = 0.f;

    std::vector<std::function<void()>> pendingSetters;

    bool loadPending = false;
    std::string loadPath = "";

    std::unordered_map<std::string, FontAtlas> m_fontAtlases;

    VkBuffer m_vb = VK_NULL_HANDLE;
    VmaAllocation m_vbAlloc = VK_NULL_HANDLE;
    size_t m_vbSize = 0;
    VkSampler m_uiSampler = VK_NULL_HANDLE;

    Widget* m_focusedWidget = nullptr;
    float m_gamepadAxisX = 0.0f;
    float m_gamepadAxisY = 0.0f;
    Widget* m_previousFocusedWidget = nullptr;
    float m_gamepadNavigationCooldown = 0.0f;
    static constexpr float GAMEPAD_NAV_COOLDOWN = 0.2f;
    static constexpr float GAMEPAD_AXIS_THRESHOLD = 0.5f;

    /// @brief Loads fonts for the UI.
    /// @param Widget* w The widget to load fonts for.
    void loadFonts(Widget* w);

    /// @brief Loads images for the UI.
    /// @param Widget* w The widget to load images for.
    void loadImages(Widget* w);

    /// @brief Layouts the UI using yoga.
    /// @param glm::uvec2 res The resolution of the UI.
    void layout(glm::uvec2 res);

    /// @brief Batches the UI for rendering.
    /// @param Widget* w The widget to batch.
    /// @param std::vector<float>& verts The vertex buffer to batch into.
    /// @param glm::vec2 parentOffset The offset of the parent widget.
    void batch(Widget* w, std::vector<float>& verts, glm::vec2 parentOffset = {0.f, 0.f});

    /// @brief Uploads the vertex buffer to the GPU.
    /// @param const std::vector<float>& verts The vertex buffer to upload.
    void uploadVerts(const std::vector<float>& verts);

    /// @brief Parses a node from json to widgets.
    /// @param const nlohmann::json& j The json node to parse.
    /// @return The parsed widget.
    Widget* parseNode(const nlohmann::json& j);

    /// @brief Frees the widget tree.
    /// @param Widget* w The widget to free.
    void freeTree(Widget* w);

    /// @brief Finds a widget at a position.
    /// @param Widget* w The widget to search.
    /// @param glm::vec2 pos The position to search at.
    /// @param glm::vec2 parentOffset The offset of the parent widget.
    /// @return The widget at the position.
    Widget* findWidgetAt(Widget* w, glm::vec2 pos, glm::vec2 parentOffset = {0.f, 0.f});

    /// @brief Finds a widget by its id.
    /// @param Widget* w The widget to search.
    /// @param const std::string& id The id to search for.
    /// @return The widget with the id.
    Widget* findById(Widget* w, const std::string& id);

    /// @brief Wraps text to fit within a given width.
    /// @param const std::string& text The text to wrap.
    /// @param const FontAtlas& a The font atlas to use.
    /// @param float maxWidth The maximum width of the text.
    /// @return A vector of wrapped lines of text.
    std::vector<std::string> wrapText(const std::string& text, const FontAtlas& a, float maxWidth);

    /// @brief Calculates the size of the text.
    /// @param Widget* w The widget to calculate the size of.
    /// @param float maxWidth The maximum width of the text.
    /// @return The size of the text.
    YGSize calculateTextSize(Widget* w, float maxWidth = FLT_MAX);

    /// @brief Measures a text node.
    /// @param const YGNode* node The node to measure.
    /// @param float width The width of the node.
    /// @param YGMeasureMode widthMode The width mode of the node.
    /// @param float height The height of the node.
    /// @param YGMeasureMode heightMode The height mode of the node.
    /// @return The size of the text.
    static YGSize measureTextNode(const YGNode* node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode);

    /// @brief Updates a widget safely.
    /// @param const std::string& id The id of the widget to update.
    /// @param std::function<void(Widget*)> action The action to perform on the widget.
    void safeUpdate(const std::string& id, std::function<void(Widget*)> action);

    /// @brief Navigate to widget in a given direction.
    /// @param float dirX Direction X component (-1, 0, or 1).
    /// @param float dirY Direction Y component (-1, 0, or 1).
    void navigateToWidget(float dirX, float dirY);

    /// @brief Get all navigable widgets (buttons).
    void getNavigableWidgets(std::vector<Widget*>& out);

    /// @brief Recursively collect navigable widgets.
    void collectNavigableWidgets(Widget* w, std::vector<Widget*>& out);

    /// @brief Check if widget is navigable (button type).
    bool isWidgetNavigable(Widget* w) const;

    /// @brief Get widget center position in screen space.
    glm::vec2 getWidgetCenter(Widget* w);

    /// @brief Find closest navigable widget in a direction.
    Widget* findClosestNavigableWidget(const glm::vec2& fromPos, const glm::vec2& direction, const std::vector<Widget*>& candidates);

    /// @brief Set focus to a widget and trigger focus callbacks.
    /// @param Widget* w - Widget to focus.
    void setFocusedWidget(Widget* w);
};

} // namespace vex
