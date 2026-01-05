#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/GameObject.hpp"

namespace vex {
class BasicCamera : public GameObject {
public:
    BasicCamera(Engine& engine, const std::string& name);
    void BeginPlay() override;
    void Update(float deltaTime) override;
private:
};
}
