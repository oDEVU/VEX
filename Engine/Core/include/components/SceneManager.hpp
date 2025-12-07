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

/// @brief Function to get all game objects.
/// @param const std::string& scene - Name of the scene to get the game objects from.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllObjects(const std::string& scene) const { return m_scenes.at(scene)->GetAllObjects(); }

/// @brief Function to get all added game objects.
/// @param const std::string& scene - Name of the scene to get the game objects from.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllAddedObjects(const std::string& scene) const { return m_scenes.at(scene)->GetAllAddedObjects(); }

/// @brief Function to get all scene names.
/// @return std::vector<std::string> - Vector of scene names.
std::vector<std::string> GetAllSceneNames() const;

/// @brief Function to get last scene name.
/// @return std::string - Last scene name.
std::string getLastSceneName() const { return lastSceneName; }

/// @brief Function to get scene by its name.
/// @param const std::string& scene - Name of the scene to get.
/// @return Scene* - Pointer to the scene.
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
