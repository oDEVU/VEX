#pragma once
#include "Engine.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

#include <components/GameObjects/CameraObject.hpp>
class EditorCameraObject : public vex::CameraObject {
public:
    EditorCameraObject(vex::Engine& engine, const std::string& name)
        : CameraObject(engine, name) {
    }

    void processEvent(const SDL_Event& event, float deltaTime);
};
