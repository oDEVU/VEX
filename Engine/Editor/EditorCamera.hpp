#pragma once
#include "Engine.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

#include <components/GameObjects/CameraObject.hpp>
class EditorCameraObject : public vex::CameraObject {
public:
    EditorCameraObject(vex::Engine& engine, const std::string& name, SDL_Window* window);

    void processEvent(const SDL_Event& event, float deltaTime);
    void Update(float deltaTime) override;

    void setViewportHovered(bool hovered) { m_viewportHovered = hovered; }
private:
    SDL_Window* m_window;
    std::unordered_map<SDL_Scancode, bool> keyStates;

    bool m_viewportHovered = false;
    bool m_isFlying = false;
    float m_mouseSensitivity = 0.1f;
    float m_movementSpeed = 50.0f;
};
