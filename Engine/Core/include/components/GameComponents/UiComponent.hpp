/**
 *  @file   UiComponent.hpp
 *  @brief  Component you add to your GameObject to add ui elements to your game. ex. hud to player.
 *  @author Eryk Roszkowski
 ***********************************************/

 #pragma once
 #include <components/UI/VexUI.hpp>

 namespace vex {
 /// @brief Struct containing VexUI class.
 struct UiComponent {
     /// @brief Constructor for UiComponent, initializes the VexUI instance.
     UiComponent(std::shared_ptr<VexUI> ui) : m_vexUI(ui) {}
     std::shared_ptr<VexUI> m_vexUI;
     bool visible = true;
 };
 } // namespace vex
