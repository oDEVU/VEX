/**
 * @file   EditorWindow.hpp
 * @brief  Defines a simple structure for a basic, detachable editor window using ImGUI.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <functional>

#include "../../Core/include/components/ImGUIWrapper.hpp"

/// @brief Defines the structure for a basic, detachable editor window.
struct BasicEditorWindow {
    bool isOpen = true;

    /**
         * @brief The function used to draw the content of the window every frame.
         * @param vex::ImGUIWrapper& - Reference to the ImGUI wrapper for rendering commands.
         */
    std::function<void(vex::ImGUIWrapper&)> Create;
};
