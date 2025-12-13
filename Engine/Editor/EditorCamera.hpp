/**
 * @file   EditorCamera.hpp
 * @brief  A specialized camera object for use within the editor, supporting free-fly movement and input processing.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "Engine.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

#include <components/GameObjects/CameraObject.hpp>

/// @brief A specialized camera object for use within the editor, supporting free-fly movement and input processing.
class EditorCameraObject : public vex::CameraObject {
public:
    /**
     * @brief Constructs an EditorCameraObject.
     * @param vex::Engine& engine - Reference to the core engine instance.
     * @param const std::string& name - The name of the camera object.
     * @param SDL_Window* window - The main SDL window pointer, used for input handling.
     */
    EditorCameraObject(vex::Engine& engine, const std::string& name, SDL_Window* window);

    /**
    * @brief Processes SDL events specific to the editor camera (e.g., mouse movement, key presses).
    * @param const SDL_Event& event - The SDL event to process.
    * @param float deltaTime - Time elapsed since the last frame.
    */
    void processEvent(const SDL_Event& event, float deltaTime);

    /// @brief Overrides the base GameObject Update for continuous camera movement and logic.
    /// @param float deltaTime - Time elapsed since the last frame.
    void Update(float deltaTime) override;

    /// @brief Sets whether the editor's viewport window is currently being hovered by the mouse.
    /// @param bool hovered - True if the viewport is hovered, false otherwise.
    void setViewportHovered(bool hovered) { m_viewportHovered = hovered; }
private:
    SDL_Window* m_window;
    std::unordered_map<SDL_Scancode, bool> keyStates;

    bool m_viewportHovered = false;
    bool m_isFlying = false;
    float m_mouseSensitivity = 0.1f;
    float m_movementSpeed = 50.0f;
};
