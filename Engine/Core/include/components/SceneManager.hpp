/**
 *  @file   SceneManager.hpp
 *  @brief  This file defines SceneManager class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <components/Scene.hpp>
#include <memory>
#include "VEX/VEX_export.h"

namespace vex {

/// @brief SceneManager class implements scene management functionality like loading and unloading scenes, holding game objects.
class VEX_EXPORT SceneManager {
public:
/// @brief Function to load a scene from a file. Simply calls clearScene() and loadSceneWithoutClearing().
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
void loadScene(const std::string& path, Engine& engine);

/// @brief Function to unload a scene from memory.
/// @param std::string& path - Path to the scene file.
void unloadScene(const std::string& path);

/// @brief Function to load a scene from a file without clearing the current scene.
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
void loadSceneWithoutClearing(const std::string& path, Engine& engine);

/// @brief Function to clear the current scene.
void clearScenes();

/// @brief Function calls update in all objects it holds. This function needs to be called in your projects update method.
/// @param float deltaTime - Delta time since last frame.
void scenesUpdate(float deltaTime);

/// @brief Function to add a game object to a scene.
/// @param const std::string& scene - Name of the scene to add the game object to.
/// @param std::shared_ptr<GameObject> gameObject - Shared pointer to the game object to add.
void AddGameObjectToScene(const std::string& scene, std::unique_ptr<GameObject> gameObject);

/// @brief Function to get game object by its unique name.
/// @param const std::string& scene - Name of the scene to get the game object from.
/// @param const std::string& name - Name of the game object to get.
/// @return std::shared_ptr<GameObject> - Shared pointer to the game object.
GameObject* GetGameObjectByName(const std::string& scene, const std::string& name);

/// @brief Function to get game object by its unique name.
/// @param const std::string& scene - Name of the scene to get the game object from.
/// @param const entt::entity& entity - Entity of the game object to get.
/// @return std::shared_ptr<GameObject> - Shared pointer to the game object.
GameObject* GetGameObjectByEntity(const std::string& scene, entt::entity& entity);

/// @brief Function to get all game objects.
/// @param const std::string& scene - Name of the scene to get the game objects from.
/// @return const std::vector<std::unique_ptr<GameObject>>& - Vector of game objects.
const std::vector<std::unique_ptr<GameObject>>& GetAllObjects(const std::string& scene) const { return m_scenes.at(scene)->GetAllObjects(); }

/// @brief Function to get all scene names.
/// @return std::vector<std::string> - Vector of scene names.
std::vector<std::string> GetAllSceneNames() const;

#if DEBUG
std::string getLastSceneName() const { return lastSceneName; }
#endif

private:
std::map<std::string, std::shared_ptr<Scene>> m_scenes;
#if DEBUG
std::string lastSceneName;
#endif
};

} // namespace vex
