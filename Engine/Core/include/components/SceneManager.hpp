/**
 *  @file   SceneManager.hpp
 *  @brief  This file defines SceneManager class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

#include "Engine.hpp"

namespace vex {

    /// @brief SceneManager class implements scene management functionality like loading and unloading scenes, holding game objects.
class SceneManager {
public:
/// @brief Function to load a scene from a file. Simply calls clearScene() and loadSceneWithoutClearing().
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
void loadScene(const std::string& path, Engine& engine);

/// @brief Function to load a scene from a file without clearing the current scene.
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
void loadSceneWithoutClearing(const std::string& path, Engine& engine);

/// @brief Function to clear the current scene.
void clearScene();

/// @brief Function calls beginGame in all objects it holds. This function needs to be called in your projects beginGame method.
void sceneBegin();
/// @brief Function calls update in all objects it holds. This function needs to be called in your projects update method.
/// @param float deltaTime - Delta time since last frame.
void sceneUpdate(float deltaTime);

/// @brief Function to get game object by its unique name.
std::shared_ptr<GameObject> GetGameObject(const std::string& name);

private:
std::vector<std::shared_ptr<GameObject>> m_objects;
};

} // namespace vex
