#pragma once

#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

#include "Engine.hpp"

namespace vex {

class SceneManager {
public:
void loadScene(const std::string& path, Engine& engine);
void loadSceneWithoutClearing(const std::string& path, Engine& engine);
void clearScene();

void sceneBegin();
void sceneUpdate(float deltaTime);

private:
std::vector<std::unique_ptr<GameObject>> m_objects;
};

} // namespace vex
