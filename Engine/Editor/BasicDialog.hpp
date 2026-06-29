/**
 * @file   basicDialog.hpp
 * @brief  Utility function to create and run a simple dialog window.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameInfo.hpp"
#include "Engine.hpp"
#include "DialogWindow.hpp"

/**
 * @brief Creates and runs a basic, modal dialog window.
 *
 * @param std::string title - The title text for the dialog window.
 * @param std::string dialog - The content message to be displayed in the dialog.
 * @param vex::GameInfo gameInfo - The GameInfo object required for initializing the DialogWindow.
 */
void createBasicDialog(std::string title, std::string dialog, vex::GameInfo gameInfo) {
    vex::DialogWindow dialogWindow(title.c_str(), gameInfo);
    dialogWindow.setDialogContent(dialog);
    dialogWindow.run();
    dialogWindow.stop();
}
