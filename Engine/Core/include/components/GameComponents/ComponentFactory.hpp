/**
 *  @file   ComponentFactory.hpp
 *  @brief  Contains ComponentRegistry class used to have register of posible to create components..
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include "components/GameObjects/GameObject.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <components/Mesh.hpp>

#if DEBUG
    #include <ImReflect.hpp>

    IMGUI_REFLECT(glm::vec3, x, y, z);
    IMGUI_REFLECT(glm::vec4, x, y, z, w);
    IMGUI_REFLECT(glm::quat, x, y, z, w);
    IMGUI_REFLECT(vex::TransformComponent, position, rotation, scale);
    IMGUI_REFLECT(vex::MeshData, meshPath);
    IMGUI_REFLECT(vex::MeshComponent, meshData, renderType, color);
    IMGUI_REFLECT(vex::LightComponent, color, intensity, radius);
    IMGUI_REFLECT(vex::CameraComponent, fov, nearPlane, farPlane);
    IMGUI_REFLECT(vex::PhysicsComponent, shape, boxHalfExtents, sphereRadius, capsuleRadius, capsuleHeight, cylinderRadius, cylinderHeight, bodyType, mass, friction, bounce, linearDamping, angularDamping, allowSleeping);
#endif

namespace vex {

/// @brief Class registering GameComponents mainly used to load them in SceneManager.
class ComponentRegistry {
public:
    using ComponentLoader = std::function<void(GameObject&, const nlohmann::json&)>;
    using ComponentInspector = std::function<void(GameObject&)>;

    static ComponentRegistry& getInstance() {
        static ComponentRegistry instance;
        return instance;
    }

    void registerComponent(const std::string& type, ComponentLoader loader, ComponentInspector inspector) {
            loaders[type] = loader;
            inspectors[type] = inspector;
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

    void drawInspectorForObject(GameObject& obj) {
            for (auto& [name, inspector] : inspectors) {
                inspector(obj);
            }
        }

    const std::unordered_map<std::string, ComponentInspector>& getInspectors() const {
        return inspectors;
    }

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentLoader> loaders;
    std::unordered_map<std::string, ComponentInspector> inspectors;
};

template<typename T>
void GenericComponentInspector(GameObject& obj) {
#if DEBUG
    if (obj.HasComponent<T>()) {
        std::string name = entt::type_id<T>().name().data();
        std::string extracted = name.substr(name.rfind("::") + 2, name.find(']') - (name.rfind("::") + 2));
        if (ImGui::CollapsingHeader(extracted.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& component = obj.GetComponent<T>();

            ImReflect::Input("##data", component);

            if (ImGui::Button("Remove")) {
                obj.GetEngine().getRegistry().remove<T>(obj.GetEntity());
            }
        }
    }
#else
    (void)obj;
#endif
}


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
                ComponentRegistry::getInstance().registerComponent( \
                    #ComponentType, \
                    LoadFunction, \
                    &vex::GenericComponentInspector<ComponentType> \
                ); \
            } \
        }; \
        static ComponentType##Registrar ComponentType##registrar; \
    }
}
