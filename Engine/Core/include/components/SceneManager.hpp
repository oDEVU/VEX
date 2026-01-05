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
/// @brief Unloads all current scenes and loads a new one from a file.
/// @details Effectively resets the game state by calling `clearScenes()` followed by `loadSceneWithoutClearing()`.
/// @param const std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance required for object creation.
void loadScene(const std::string& path, Engine& engine);

/// @brief Unloads a specific scene from memory.
/// @details Removes the scene from the internal `m_scenes` map, triggering the destructor of the `Scene` object.
/// @param const std::string& path - Path/Name of the scene to unload.
void unloadScene(const std::string& path);

/// @brief Loads a scene from a file additively without clearing existing scenes.
/// @details
/// 1. Updates `lastSceneName` to the provided path.
/// 2. Creates a new `Scene` shared pointer.
/// 3. Calls `sceneBegin()` on the newly created scene to initialize its objects.
/// @param const std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
void loadSceneWithoutClearing(const std::string& path, Engine& engine);

/// @brief Function to clear the current scene.
void clearScenes();

/// @brief Updates all currently loaded scenes.
/// @details Iterates through the `m_scenes` map and calls `sceneUpdate(deltaTime)` on each active scene.
/// @param float deltaTime - Delta time since the last frame.
void scenesUpdate(float deltaTime);

/// @brief Function to get all game objects.
/// @param const std::string& scene - Name of the scene to get the game objects from.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllObjects(const std::string& scene) const { return m_scenes.at(scene)->GetAllObjects(); }

/// @brief Function to get all added game objects.
/// @param const std::string& scene - Name of the scene to get the game objects from.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllAddedObjects(const std::string& scene) const { return m_scenes.at(scene)->GetAllAddedObjects(); }

/// @brief Retrieves the names of all currently loaded scenes.
/// @details Iterates through the internal `m_scenes` map keys to construct the list.
/// @return std::vector<std::string> - Vector containing the names/paths of active scenes.
std::vector<std::string> GetAllSceneNames() const;

/// @brief Function to get last scene name.
/// @return std::string - Last scene name.
std::string getLastSceneName() const { return lastSceneName; }

/// @brief Safely retrieves a pointer to a specific Scene.
/// @details Checks if the scene exists in the `m_scenes` map before returning.
/// @param const std::string& scene - Name/Path of the scene to retrieve.
/// @return Scene* - Pointer to the scene if found, otherwise `nullptr`.
Scene* GetScene(const std::string& scene) const {
    if(m_scenes.contains(scene)){
        return m_scenes.at(scene).get();
    }
    return nullptr;
}

private:
std::map<std::string, std::shared_ptr<Scene>> m_scenes;
std::string lastSceneName = "";
};

} // namespace vex
