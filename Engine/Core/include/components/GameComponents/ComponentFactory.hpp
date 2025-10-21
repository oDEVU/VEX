/**
 *  @file   ComponentFactory.hpp
 *  @brief  Contains ComponentRegistry class used to have register of posible to create components..
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>

namespace vex {

class GameObject;

/// @brief Class registering GameComponents mainly used to load them in SceneManager.
class ComponentRegistry {
public:
    using ComponentLoader = std::function<void(GameObject&, const nlohmann::json&)>;

    static ComponentRegistry& getInstance() {
        static ComponentRegistry instance;
        return instance;
    }

    void registerComponent(const std::string& type, ComponentLoader loader) {
        loaders[type] = loader;
    }

    bool loadComponent(GameObject& obj, const std::string& type, const nlohmann::json& json) {
        auto it = loaders.find(type);
        if (it != loaders.end()) {
            it->second(obj, json);
            return true;
        }
        log("Error: Component type '%s' not registered", type.c_str());
        return false;
    }

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentLoader> loaders;
};

/// @brief Macro used to register GameComponents in ComponentRegistry. It allows to add component from scene file.
/// @details Example usage:
/// @code
/// void LoadCameraComponent(GameObject& obj, const nlohmann::json& json) {
///     CameraComponent comp;
///     if (json.contains("fov") && !json["fov"].is_array()) {
///         comp.fov = json["fov"].get<float>();
///     }
///     if (json.contains("nearPlane") && !json["nearPlane"].is_array()) {
///         comp.nearPlane = json["nearPlane"].get<float>();
///     }
///     if (json.contains("farPlane") && !json["farPlane"].is_array()) {
///         comp.farPlane = json["farPlane"].get<float>();
///     }
///     obj.AddComponent(comp);
/// }
///
/// REGISTER_COMPONENT(CameraComponent, LoadCameraComponent);
/// @endcode
#define REGISTER_COMPONENT(ComponentType, LoadFunction) \
    namespace { \
        struct ComponentType##Registrar { \
            ComponentType##Registrar() { \
                ComponentRegistry::getInstance().registerComponent(#ComponentType, LoadFunction); \
            } \
        }; \
        static ComponentType##Registrar ComponentType##registrar; \
    }
}
