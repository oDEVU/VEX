#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

#include "../../Core/include/components/ImGUIWrapper.hpp"
#include "Engine.hpp"

#include "EditorWindow.hpp"

class EditorMenuBar{
public:
    EditorMenuBar(vex::ImGUIWrapper& imGUIWrapper, vex::Engine& m_engine) : m_ImGUIWrapper(imGUIWrapper), m_engine(m_engine) {};
    ~EditorMenuBar() {};
    void DrawBar();
    void OpenScene();
private:
    vex::ImGUIWrapper& m_ImGUIWrapper;
    vex::Engine& m_engine;
    std::vector<std::shared_ptr<BasicEditorWindow>> m_Windows;
};
