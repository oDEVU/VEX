/**
 *  @file   ComponentFactory.hpp
 *  @brief  Contains ComponentRegistry class used to have register of posible to create components..
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include "components/GameObjects/GameObject.hpp"
#include "components/colorTypes.hpp"
#include "SerializationUtils.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <components/Mesh.hpp>

#if DEBUG
    #include <ImReflect.hpp>

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::vec4& value, ImSettings& settings, ImResponse& response) {
        ImGui::DragFloat4(label, &value.x);
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::vec3& value, ImSettings& settings, ImResponse& response) {
        ImGui::DragFloat3(label, &value.x);
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::vec2& value, ImSettings& settings, ImResponse& response) {
        ImGui::DragFloat2(label, &value.x);
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, vex::rgb& value, ImSettings& settings, ImResponse& response) {
        ImGui::ColorEdit3(label, &value.x);
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, vex::rgba& value, ImSettings& settings, ImResponse& response) {
        ImGui::ColorEdit4(label, &value.x);
    }

    IMGUI_REFLECT(vex::rgb, x, y, z);
    IMGUI_REFLECT(vex::rgba, x, y, z, w);
    IMGUI_REFLECT(glm::vec2, x, y);
    IMGUI_REFLECT(glm::vec3, x, y, z);
    IMGUI_REFLECT(glm::vec4, x, y, z, w);
    IMGUI_REFLECT(vex::MeshData, meshPath);

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::quat& value, ImSettings& settings, ImResponse& response) {
        glm::vec3 euler = glm::degrees(glm::eulerAngles(value));
        if (ImGui::DragFloat3(label, &euler.x)) {
            value = glm::quat(glm::radians(euler));
        }
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::ivec2& value, ImSettings& settings, ImResponse& response) {
        ImGui::DragInt2(label, &value.x);
    }
    inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::ivec3& value, ImSettings& settings, ImResponse& response) {
        ImGui::DragInt3(label, &value.x);
    }

    IMGUI_REFLECT(glm::quat, x, y, z, w);
    IMGUI_REFLECT(glm::ivec2, x, y);
    IMGUI_REFLECT(glm::ivec3, x, y, z);
#else
    #define IMGUI_REFLECT(...);
#endif

namespace vex {

    template<typename T>
    void GenericComponentInspector(GameObject& obj) {
    #if DEBUG
        if (obj.HasComponent<T>()) {
            std::string name = entt::type_id<T>().name().data();
            std::string extracted = name.substr(name.rfind("::") + 2, name.find(']') - (name.rfind("::") + 2));

            ImGui::PushID(name.c_str());
            if (ImGui::CollapsingHeader(extracted.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& component = obj.GetComponent<T>();

                ImReflect::Input("##data", component);

                ImGui::Dummy(ImVec2(5.0f, 5.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.47f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.71f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.03f, 0.03f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.85f, 0.85f, 1.0f)); // White Text

                if (ImGui::Button("Remove")) {
                    obj.GetEngine().getRegistry().remove<T>(obj.GetEntity());
                }

                ImGui::PopStyleColor(4);
            }
            ImGui::PopID();
        }
    #else
        (void)obj;
    #endif
    }

/// @brief Class registering GameComponents mainly used to load them in SceneManager.
class ComponentRegistry {
public:
    using ComponentLoader = std::function<void(GameObject&, const nlohmann::json&)>;
    using ComponentSaver = std::function<nlohmann::json(GameObject&)>;
    using ComponentInspector = std::function<void(GameObject&)>;
    using ComponentChecker = std::function<bool(const GameObject&)>;
    using ComponentCreator = std::function<void(GameObject&)>;

    static ComponentRegistry& getInstance() {
        static ComponentRegistry instance;
        return instance;
    }

    // Generic Register Function
        template<typename T>
        void registerComponent(const std::string& name) {
            // 1. Loader: Relies on from_json(json, T) being defined (either Auto or Manual)
            loaders[name] = [](GameObject& obj, const nlohmann::json& j) {
                if (!obj.HasComponent<T>()) {
                     // Try default construction. Note: TransformComponent needs handling if no default ctor.
                     // Assuming you added a default ctor or logic to handle it.
                     if constexpr (std::is_default_constructible_v<T>) {
                         obj.AddComponent<T>(T());
                     }
                }
                if (obj.HasComponent<T>()) {
                    T& comp = obj.GetComponent<T>();
                    j.get_to(comp); // Calls global from_json
                }
            };

            // 2. Saver: Relies on to_json(json, T) being defined
            savers[name] = [](GameObject& obj) -> nlohmann::json {
                if (obj.HasComponent<T>()) {
                    return obj.GetComponent<T>(); // Calls global to_json
                }
                return nullptr;
            };

            // 3. Inspector: Registers the Generic template.
            // If you specialize this template for a type (like PhysicsComponent),
            // the compiler will pick your specialization automatically.
            inspectors[name] = &GenericComponentInspector<T>;

            checkers[name] = [](const GameObject& obj) {
                return obj.HasComponent<T>();
            };

            creators[name] = [](GameObject& obj) {
                if (!obj.HasComponent<T>()) {
                    if constexpr (std::is_default_constructible_v<T>) {
                        obj.AddComponent<T>(T());
                    } else {
                        log("Error: Component '%s' is not default constructible.", typeid(T).name());
                    }
                }
            };

            registeredNames.push_back(name);
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

    nlohmann::json saveComponent(GameObject& obj, const std::string& type) {
        if (savers.find(type) != savers.end()) return savers[type](obj);
        return nullptr;
    }

    void drawInspectorForObject(GameObject& obj) {
            for (auto& [name, inspector] : inspectors) {
                inspector(obj);
            }
        }

        std::vector<std::string> getMissingComponents(const GameObject& obj) {
                std::vector<std::string> missing;
                for (const auto& name : registeredNames) {
                    if (checkers.find(name) != checkers.end() && !checkers[name](obj)) {
                        missing.push_back(name);
                    }
                }
                return missing;
            }

            void createComponent(GameObject& obj, const std::string& name) {
                    if (creators.find(name) != creators.end()) {
                        creators[name](obj);
                    } else {
                        log("Error: Cannot create component '%s', not registered.", name.c_str());
                    }
                }

    const std::unordered_map<std::string, ComponentInspector>& getInspectors() const {
        return inspectors;
    }

    const std::vector<std::string>& getRegisteredNames() const { return registeredNames; }

private:
    ComponentRegistry() = default;
    std::vector<std::string> registeredNames;
    std::unordered_map<std::string, ComponentLoader> loaders;
    std::unordered_map<std::string, ComponentSaver> savers;
    std::unordered_map<std::string, ComponentInspector> inspectors;
    std::unordered_map<std::string, ComponentChecker> checkers;
    std::unordered_map<std::string, ComponentCreator> creators;
};

}

/// @brief Helper macros to generate unique names based on line number
#define VEX_CAT_IMPL(a, b) a##b
#define VEX_CAT(a, b) VEX_CAT_IMPL(a, b)
#define VEX_UNIQUE_NAME(prefix) VEX_CAT(prefix, __LINE__)

/// @brief Macro used to register GameComponents in ComponentRegistry. It allows to add component from scene file.
/// @details Example usage:
/// @code
/// REGISTER_COMPONENT(CameraComponent, fov, nearPlane, farPlane);
/// @endcode
#define REGISTER_COMPONENT(Type, ...) \
    namespace vex { NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Type, __VA_ARGS__) } \
    IMGUI_REFLECT(Type, __VA_ARGS__) \
    namespace { \
        struct VEX_UNIQUE_NAME(Registrar_) { \
            VEX_UNIQUE_NAME(Registrar_)() { \
                vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type); \
            } \
        }; \
        static VEX_UNIQUE_NAME(Registrar_) VEX_UNIQUE_NAME(g_registrar_); \
    }

/// @brief Same macro just skips json registration for more complex components, it requires manual registration.
#define REGISTER_COMPONENT_CUSTOM(Type, ...) \
    IMGUI_REFLECT(Type, __VA_ARGS__) \
    namespace { \
        struct VEX_UNIQUE_NAME(Registrar_) { \
            VEX_UNIQUE_NAME(Registrar_)() { \
                vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type); \
            } \
        }; \
        static VEX_UNIQUE_NAME(Registrar_) VEX_UNIQUE_NAME(g_registrar_); \
    }
