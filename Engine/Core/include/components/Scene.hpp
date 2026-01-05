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

/// @brief Calls `BeginPlay` on all objects in the scene.
/// @details Iterates through both main storage (`m_objects`) and the added queue (`m_addedObjects`).
/// If `VEX_USE_PARALLEL_EXECUTION` is defined and object count > 50, execution is parallelized.
void sceneBegin();

/// @brief Updates all objects in the scene.
/// @details
/// 1. Flushes the destruction queue via `FlushDestructionQueue`.
/// 2. Calls `Update(deltaTime)` on all active objects.
/// 3. Supports parallel execution for large object counts if configured.
/// @param float deltaTime - Delta time since last frame.
void sceneUpdate(float deltaTime);

/// @brief [Deprecated] Function to add a game object to the scene.
/// @param std::unique_ptr<GameObject> gameObject - Shared pointer to the game object to add.
void AddGameObject(std::unique_ptr<GameObject> gameObject);

/// @brief Function to get all game objects with a specific name.
/// @param const std::string& name - Name of the game object to get.
/// @return std::vector<GameObject*> - Vector of pointers to the matching game objects.
std::vector<GameObject*> GetAllGameObjectsByName(const std::string& name);

/// @brief Function to get all game objects of a specific class type.
/// @param const std::string& classname - Class name in string form.
/// @return std::vector<GameObject*> - Vector of pointers to the matching game objects.
std::vector<GameObject*> GetAllGameObjectsByClassName(const std::string& classname);

/// @brief Function to get a game object by its entity ID.
/// @param entt::entity& entity - Entity of the game object to get.
/// @return GameObject* - Pointer to the game object, or nullptr if not found.
GameObject* GetGameObjectByEntity(entt::entity& entity);

/// @brief Registers a game object into the scene's internal storage.
/// @details Places the object into `m_objects` if loading from a scene file, or `m_addedObjects` if created at runtime.
/// @param GameObject* gameObject - Pointer to the game object to register.
void RegisterGameObject(GameObject* gameObject);

/// @brief Marks a game object for destruction.
/// @details Adds the object to `m_pendingDestruction` to be cleaned up at the start of the next `sceneUpdate`.
/// @param GameObject* gameObject - Pointer to the game object to destroy.
void DestroyGameObject(GameObject* gameObject);

/// @brief Function to get all game objects in the scene.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Reference to the vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllObjects() const { return m_objects; }

/// @brief Function to get all added game objects in the scene.
/// @return const std::vector<std::shared_ptr<GameObject>>& - Reference to the vector of game objects.
const std::vector<std::shared_ptr<GameObject>>& GetAllAddedObjects() const { return m_addedObjects; }

/// @brief Saves the current state of the scene to a JSON file.
/// @details Serializes:
/// - Environment settings (Sun light, Ambient light, Shading artifacts).
/// - All GameObjects and their components (using `ComponentRegistry::saveComponent`).
/// - Object hierarchy (Parent/Child relationships).
/// - Sorts objects to ensure parents are written before children to maintain hierarchy integrity on load.
/// @param const std::string& outputPath - The filesystem path to write the .json file to.
void Save(const std::string& outputPath);

/// @brief Promotes a temporary GameObject (e.g., created by the Editor) to a persistent Scene object.
/// @details Moves the object from the temporary `m_addedObjects` list to the primary `m_objects` storage, ensuring it is saved with the scene.
/// @param GameObject* gameObject - Pointer to the object to promote.
void AddEditorGameObject(GameObject* gameObject);

std::string GetScenePath(){
    return m_path;
}

/// @brief Processes the queue of objects marked for destruction.
/// @details Waits for the GPU to finish operations via `Engine::WaitForGpu` to ensure safe deletion, then removes the objects from internal storage vectors.
void FlushDestructionQueue();

private:

/// @brief Loads the scene data from the file path specified in the constructor.
/// @details
/// 1. Reads the JSON file from the Virtual File System.
/// 2. Parses and applies Global Environment settings (Lighting, Shading, Dithering).
/// 3. Iterates through the "objects" array, creating GameObjects via `GameObjectFactory`.
/// 4. Loads components for each object via `ComponentRegistry`.
/// 5. Reconstructs parent-child hierarchies based on name references.
void load();

std::vector<std::shared_ptr<GameObject>> m_objects;
std::vector<std::shared_ptr<GameObject>> m_addedObjects;
std::vector<GameObject*> m_pendingDestruction;
std::string m_path;
Engine* m_engine;
bool m_creatingFromScene = false;
};

} // namespace vex
