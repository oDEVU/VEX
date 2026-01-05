#pragma once

#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/CameraObject.hpp"

namespace vex {
    class Player : public GameObject {
    public:
        Player(Engine& engine, const std::string& name);
        void BeginPlay() override;
        void Update(float deltaTime) override;
    private:
        float movementSpeed = 10.0f;
        CameraObject* camera;
    };
}
