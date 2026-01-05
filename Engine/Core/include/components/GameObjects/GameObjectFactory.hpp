/**
 * @file   GameObjectFactory.hpp
 * @brief  This file defines GameObjectFactory class used to auto register GameObjects.
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

#include "VEX/VEX_export.h"

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

namespace vex {

///@brief Class responsible for registering and then creating GameObjects eg. Player, CameraObject. It is needed to load GameObjects from scene files but not all GameObjects can be registered. It requires your GameObject to not change constructor parameters.
class VEX_EXPORT GameObjectFactory {
public:
    using ObjectConstructor = GameObject* (*)(Engine&, const std::string&);

    using Creator = std::function<GameObject*(Engine&, const std::string&)>;

    ///@brief Get the singleton instance of the GameObjectFactory.
    static GameObjectFactory& getInstance();

    /// @brief constructs registered objects, used internally in scene loading.
    template<typename T>
    static GameObject* constructObject(Engine& engine, const std::string& name) {
        return new T(engine, name);
    }

    /// @brief Register a GameObject class with the factory.
    /// @param std::string& name The name of the GameObject class.
    /// @param bool isDynamic Whether the GameObject class is dynamic.
    template<typename T>
    void registerClass(const std::string& name, bool isDynamic = false) {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");

        registerCreator(name, &constructObject<T>, isDynamic);
    }

    /// @brief Create a GameObject instance of the specified type string.
    /// @details Looks up the creator in the `creators` map. If found, invokes it. If not found, logs an error and returns `nullptr`.
    /// @param const std::string& type - The registered type string.
    /// @param Engine& engine - The engine instance to pass to the constructor.
    /// @param const std::string& name - The name for the new GameObject instance.
    /// @return GameObject* - Pointer to the created object or nullptr on failure.
    GameObject* create(const std::string& type, Engine& engine, const std::string& name);

    /// @brief Get all registered object types.
    /// @return std::vector<std::string> The registered object types.
    std::vector<std::string> GetAllRegisteredObjectTypes();

    /// @brief Get all non-dynamic registered object types.
    /// @return std::vector<std::string> The non-dynamic registered object types.
    std::vector<std::string> GetNonDynamicRegisteredObjectTypes();

    /// @brief Get all dynamic registered object types.
    /// @return std::vector<std::string> The dynamic registered object types.
    std::vector<std::string> GetDynamicRegisteredObjectTypes();

    /// @brief Clears all GameObjects registered as dynamic.
    /// @details Used during hot-reload. Iterates through `dynamicTypes`, removes them from the `creators` map, and resets the `allTypes` list.
    void clearDynamicGameObjects();

    /// @brief Unregisters all GameObjects and clears internal lists.
    /// @details Clears `creators`, `dynamicTypes`, `nonDynamicTypes`, and `allTypes`.
    void UnregisterGameObjects();

private:
    GameObjectFactory() = default;

    /// @brief Registers a creator function for a specific GameObject type string.
    /// @details Stores a lambda in the `creators` map that constructs the object and sets its `objectType`.
    /// Appends the name to `allTypes` and either `dynamicTypes` or `nonDynamicTypes` based on the flag.
    /// @param const std::string& name - The string identifier for the class.
    /// @param ObjectConstructor ctor - Function pointer to the constructor.
    /// @param bool isDynamic - True if this class is part of a hot-reloadable module.
    void registerCreator(const std::string& name, ObjectConstructor ctor, bool isDynamic);

    std::unordered_map<std::string, Creator> creators;
    std::vector<std::string> dynamicTypes;
    std::vector<std::string> nonDynamicTypes;
    std::vector<std::string> allTypes;
};

/// @brief Macro for automatic GameObject registration, you should call it at the bottom of your class implementation.
/// @details Example usage:
/// @code
/// class MyGameObject : public GameObject {
/// public:
///     MyGameObject(Engine& engine, const std::string& name) : GameObject(engine, name) {}
/// };
///
/// REGISTER_GAME_OBJECT(MyGameObject);
/// @endcode
#ifdef GAME_MODULE_EXPORTS
    #define REGISTER_GAME_OBJECT(ClassName) \
        namespace { \
            struct ClassName##Registrar { \
                ClassName##Registrar() { \
                    GameObjectFactory::getInstance().registerClass<ClassName>(#ClassName, true); \
                } \
            }; \
            static ClassName##Registrar ClassName##registrar; \
        }
#else
    #define REGISTER_GAME_OBJECT(ClassName) \
        namespace { \
            struct ClassName##Registrar { \
                ClassName##Registrar() { \
                    GameObjectFactory::getInstance().registerClass<ClassName>(#ClassName, false); \
                } \
            }; \
            static ClassName##Registrar ClassName##registrar; \
        }
#endif

}
