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

    static GameObjectFactory& getInstance();

    template<typename T>
    static GameObject* constructObject(Engine& engine, const std::string& name) {
        return new T(engine, name);
    }

    template<typename T>
    void registerClass(const std::string& name, bool isDynamic = false) {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");

        registerCreator(name, &constructObject<T>, isDynamic);
    }

    GameObject* create(const std::string& type, Engine& engine, const std::string& name);

    std::vector<std::string> GetAllRegisteredObjectTypes();
    std::vector<std::string> GetNonDynamicRegisteredObjectTypes();
    std::vector<std::string> GetDynamicRegisteredObjectTypes();

    void clearDynamicGameObjects();

    void UnregisterGameObjects();

private:
    GameObjectFactory() = default;

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
