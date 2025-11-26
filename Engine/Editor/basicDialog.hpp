#pragma once

#include "components/GameInfo.hpp"
#include "Engine.hpp"
#include "DialogWindow.hpp"

void createBasicDialog(std::string title, std::string dialog, vex::GameInfo gameInfo) {
    vex::DialogWindow dialogWindow(title.c_str(), gameInfo);
    dialogWindow.setDialogContent(dialog);
    dialogWindow.run();
    dialogWindow.stop();
}
