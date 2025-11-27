#pragma once

#include <functional>

#include "../../Core/include/components/ImGUIWrapper.hpp"

struct BasicEditorWindow {
    bool isOpen = true;
    std::function<void(vex::ImGUIWrapper&)> Create;
};
