#pragma once
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>

namespace vex {

class VexUI;

enum class FlexDirection { Row, Column };
enum class PositionType { Relative, Absolute };
enum class Justify { FlexStart, Center, FlexEnd, SpaceBetween };
enum class Align { Auto, FlexStart, Center, FlexEnd, Stretch, Baseline };
enum class FlexWrap { NoWrap, Wrap, WrapReverse };

/// @brief Lightweight struct to hold parsed sizes safely away from VexUI dependencies.
struct FlexValue {
    float v = NAN; ///< Raw value
    bool isPercent = false; ///< Whether value is a percentage

    /// @brief Resolve the flex value to pixels based on parent dimension.
    /// @param parent Parent dimension in pixels.
    /// @return Resolved value in pixels, or NAN if unspecified.
    float resolve(float parent) const {
        if (std::isnan(v)) return NAN;
        return isPercent ? (v / 100.0f) * parent : v;
    }

    /// @brief Resolve the flex value to pixels, with a fallback if unspecified.
    /// @param parent Parent dimension in pixels.
    /// @param fallback Value to return if resolution results in NAN.
    /// @return Resolved value in pixels, or fallback if unspecified.
    float resolveOr(float parent, float fallback) const {
        float res = resolve(parent);
        return std::isnan(res) ? fallback : res;
    }
};

/// @brief Layout node representing a single UI element in the layout tree.
struct LayoutNode {
    LayoutNode* parent = nullptr; ///< Parent node in the layout tree
    std::vector<LayoutNode*> children; ///< Child nodes
    void* context = nullptr; ///< Opaque context pointer for user data

    /// @name Dimensional Properties
    /// @{
    FlexValue width; ///< Element width
    FlexValue height; ///< Element height
    /// @}

    /// @name Positional Properties
    /// @{
    FlexValue left; ///< Left position offset
    FlexValue top; ///< Top position offset
    FlexValue right; ///< Right position offset
    FlexValue bottom; ///< Bottom position offset
    /// @}

    /// @name Margin Properties
    /// @{
    FlexValue marginLeft; ///< Left margin
    FlexValue marginTop; ///< Top margin
    FlexValue marginRight; ///< Right margin
    FlexValue marginBottom; ///< Bottom margin
    /// @}

    /// @name Padding Properties
    /// @{
    FlexValue paddingLeft; ///< Left padding
    FlexValue paddingTop; ///< Top padding
    FlexValue paddingRight; ///< Right padding
    FlexValue paddingBottom; ///< Bottom padding
    /// @}

    /// @name Border Properties
    /// @{
    FlexValue borderWidth; ///< Border width
    /// @}

    /// @name Flex Properties
    /// @{
    float flexGrow = 0.0f; ///< Flex grow factor
    float flexShrink = 1.0f; ///< Flex shrink factor
    /// @}

    /// @name Layout Properties
    /// @{
    FlexDirection flexDirection = FlexDirection::Column; ///< Direction of flex layout
    PositionType positionType = PositionType::Relative; ///< Position type (relative or absolute)
    Justify justifyContent = Justify::FlexStart; ///< Justify content alignment
    Align alignItems = Align::Stretch; ///< Align items cross-axis alignment
    Align alignSelf = Align::Auto; ///< Align self override
    FlexWrap flexWrap = FlexWrap::NoWrap; ///< Flex wrap behavior
    /// @}

    /// @brief Measure function for intrinsic sizing (e.g., text).
    std::function<void(LayoutNode*, float, float)> measureFunc = nullptr;

    /// @name Computed Layout Values
    /// @{
    float computedLeft = 0.0f; ///< Computed left position
    float computedTop = 0.0f; ///< Computed top position
    float computedWidth = 0.0f; ///< Computed width
    float computedHeight = 0.0f; ///< Computed height
    /// @}

    /// @brief Insert a child node at a specific index.
    /// @param child The child node to insert.
    /// @param index The position to insert at.
    void insertChild(LayoutNode* child, size_t index) {
        child->parent = this;
        children.insert(children.begin() + index, child);
    }
};

/// @brief Calculate the layout of a node tree using flex layout algorithm.
/// @param node The root node to calculate layout for.
/// @param parentWidth The available parent width in pixels.
/// @param parentHeight The available parent height in pixels.
/// @param isRoot Whether this is the root node (defaults to false).
/// @param ui Pointer to VexUI instance for rendering context (defaults to nullptr).
/// @param forcedW Forced width constraint in pixels (defaults to NAN for no constraint).
/// @param forcedH Forced height constraint in pixels (defaults to NAN for no constraint).
void calculateLayout(LayoutNode* node, float parentWidth, float parentHeight, bool isRoot = false, const VexUI* ui = nullptr, float forcedW = NAN, float forcedH = NAN);
} // namespace vex