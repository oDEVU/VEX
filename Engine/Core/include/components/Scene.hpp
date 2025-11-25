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

namespace vex {

/// @brief Scene class implements scene functionality like loading and holding game objects.
class VEX_EXPORT Scene {
public:
/// @brief Constructor loads a scene from a file.
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

/// @brief Function to add a game object to the scene.
/// @param std::unique_ptr<GameObject> gameObject - Shared pointer to the game object to add.
void AddGameObject(std::unique_ptr<GameObject> gameObject);

/// @brief Function to get game object by its unique name.
/// @param const std::string& name - Name of the game object to get.
/// @return GameObject* - Shared pointer to the game object.
GameObject* GetGameObjectByName(const std::string& name);

/// @brief Function to get game object by its entity.
/// @param entt::entity& entity - Entity of the game object to get.
/// @return GameObject* - Shared pointer to the game object.
GameObject* GetGameObjectByEntity(entt::entity& entity);

private:
std::vector<std::unique_ptr<GameObject>> m_objects;
};

} // namespace vex
