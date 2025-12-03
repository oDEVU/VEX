#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

#include "../../Core/include/components/ImGUIWrapper.hpp"
#include "Engine.hpp"

#include "EditorWindow.hpp"

namespace vex { class Editor; }

class EditorMenuBar{
public:
    EditorMenuBar(vex::ImGUIWrapper& imGUIWrapper, vex::Editor& editor) : m_ImGUIWrapper(imGUIWrapper), m_editor(editor) {};
    ~EditorMenuBar() {};
    void DrawBar();
    void OpenScene();
    void OpenEditorSettings();
    void SaveSceneAs();
private:
    vex::ImGUIWrapper& m_ImGUIWrapper;
    vex::Editor& m_editor;
    std::vector<std::shared_ptr<BasicEditorWindow>> m_Windows;
};
