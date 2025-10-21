/**
 *  @file   GameObjectFactory.hpp
 *  @brief  This file defines GameObjectFactory class used to auto register GameObjects.
 *  @author Eryk Roszkowski
 ***********************************************/


#pragma once
#include "Engine.hpp"
#include "components/errorUtils.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/GameComponents/BasicComponents.hpp"

#include <functional>
#include <unordered_map>
#include <string>

namespace vex {
///@brief Class responsible for registering and then creating GameObjects eg. Player, CameraObject. It is needed to load GameObjects from scene files but not all GameObjects can be registered. It requires your GameObject to not change constructor parameters.
class GameObjectFactory {
public:
    using Creator = std::function<GameObject*(Engine&, const std::string&)>;

    static GameObjectFactory& getInstance() {
        static GameObjectFactory instance;
        return instance;
    }

    template<typename T>
    void registerClass(const std::string& name) {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
        creators[name] = [](Engine& e, const std::string& n) { return new T(e, n); };
    }

    GameObject* create(const std::string& type, Engine& engine, const std::string& name) {
        auto it = creators.find(type);
        if (it != creators.end()) {
            return it->second(engine, name);
        }
        log("Error: Type '%S' not registered", type.c_str());
        return nullptr;
    }

private:
    GameObjectFactory() = default;
    std::unordered_map<std::string, Creator> creators;
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
#define REGISTER_GAME_OBJECT(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                GameObjectFactory::getInstance().registerClass<ClassName>(#ClassName); \
            } \
        }; \
        static ClassName##Registrar ClassName##registrar; \
    }
}
