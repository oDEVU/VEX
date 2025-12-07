/**
 *  @file   Scene.hpp
 *  @brief  This file defines SceneManager class.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include "components/GameObjects/GameObject.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"

#include "Engine.hpp"
#include "entt/entity/fwd.hpp"

#include "VEX/VEX_export.h"
#include <algorithm>
#include <memory>

namespace vex {

/// @brief Scene class implements scene functionality like loading and holding game objects.
class VEX_EXPORT Scene {
public:
/// @brief Constructor creates empty scene object.
/// @param std::string& path - Path to the scene file.
/// @param Engine& engine - Reference to the engine instance.
Scene(const std::string& path, Engine& engine);

Scene(const Scene&) = delete;
Scene& operator=(const Scene&) = delete;

Scene(Scene&&) = default;
Scene& operator=(Scene&&) = default;

/// @brief Destructor clears the current scene.
~Scene();

/// @brief Function calls beginGame in all objects it holds. This function needs to be called in your projects beginGame method.
void sceneBegin();
/// @brief Function calls update in all objects it holds. This function needs to be called in your projects update method.
/// @param float deltaTime - Delta time since last frame.
void sceneUpdate(float deltaTime);

/// @brief [Deprecated] Function to add a game object to the scene.
/// @param std::unique_ptr<GameObject> gameObject - Shared pointer to the game object to add.
void AddGameObject(std::unique_ptr<GameObject> gameObject);

/// @brief Function to get all game objects with name.
/// @param const std::string& name - Name of the game object to get.
/// @return std::vector<GameObject*> - vector of pointers to the game object.
std::vector<GameObject*> GetAllGameObjectsByName(const std::string& name);

/// @brief Function to get all gameobject of class
/// @param const std::string classname - class name in string form.
/// @return std::vector<GameObject*> - vector of pointers to the game object.
std::vector<GameObject*> GetAllGameObjectsByClassName(const std::string& classname);

/// @brief Function to get game object by its entity.
/// @param entt::entity& entity - Entity of the game object to get.
/// @return GameObject* - pointer to the game object.
GameObject* GetGameObjectByEntity(entt::entity& entity);

/// @brief Function to register a game object in the scene.
/// @param GameObject* gameObject - Pointer to the game object to register.
void RegisterGameObject(GameObject* gameObject);

/// @brief Function to destroy a game object in the scene.
/// @param GameObject* gameObject - Pointer to the game object to destroy.
void DestroyGameObject(GameObject* gameObject);

/// @brief Function to get all game objects in the scene.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Reference to the vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllObjects() const { return m_objects; }

/// @brief Function to get all added game objects in the scene.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Reference to the vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllAddedObjects() const { return m_addedObjects; }

/// @brief Function to save the scene to a file path.
/// @param const std::string& outputPath - Path to save the scene to.
void Save(const std::string& outputPath);

/// @brief Function to add a game object specifically for Editor usage (saved to file).
/// @param GameObject* gameObject - Pointer to the object.
void AddEditorGameObject(GameObject* gameObject);

std::string GetScenePath(){
    return m_path;
}

private:

/// @brief Function to load the scene from a file path saved in the constructor.
void load();

std::vector<std::shared_ptr<GameObject>> m_objects;
std::vector<std::shared_ptr<GameObject>> m_addedObjects;
std::string m_path;
Engine* m_engine;
bool m_creatingFromScene = false;
};

} // namespace vex
