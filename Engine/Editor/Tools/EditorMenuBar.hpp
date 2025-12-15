/**
 * @file   EditorMenuBar.hpp
 * @brief  ImGUI component responsible for drawing the main editor menu bar and handling its actions.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

#include "../../Core/include/components/ImGUIWrapper.hpp"
#include "Engine.hpp"

#include "EditorWindow.hpp"

namespace vex { class Editor; }

/// @brief ImGUI component responsible for drawing the main editor menu bar and handling its actions.
class EditorMenuBar{
public:
/**
     * @brief Constructs the EditorMenuBar.
     * @param vex::ImGUIWrapper& imGUIWrapper - Reference to the ImGUI wrapper for rendering context.
     * @param vex::Editor& editor - Reference to the main Editor instance to execute commands.
     */
    EditorMenuBar(vex::ImGUIWrapper& imGUIWrapper, vex::Editor& editor) : m_ImGUIWrapper(imGUIWrapper), m_editor(editor) {};
    ~EditorMenuBar() {};

    /// @brief Draws the main menu bar and processes user input.
    void DrawBar();

    /// @brief Handles the logic for opening an existing scene file.
    void OpenScene();

    /// @brief Opens the editor settings window/menu.
    void OpenEditorSettings();

    /// @brief Opens the project settings window/menu.
    void OpenProjectSettings();

    /// @brief Saves the current scene to a new file path.
    void SaveSceneAs();

    /**
    * @brief Executes the project build process.
         * @param bool isDebug - True for a debug build, false for a release build.
         * @param bool runAfter - True to run the built executable immediately after a successful build.
         */
    void RunBuild(bool isDebug, bool runAfter = false);

    /// @brief Copies empty scene to assets folders and opens it.
    void NewScene();
private:
    /// @brief Retrieves the name of the current project.
    /// @return std::string - The project name.
    std::string GetProjectName();
    vex::ImGUIWrapper& m_ImGUIWrapper;
    vex::Editor& m_editor;
    std::vector<std::shared_ptr<BasicEditorWindow>> m_Windows;
};
