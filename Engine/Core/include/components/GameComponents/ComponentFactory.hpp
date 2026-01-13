/**
 * @file   ComponentFactory.hpp
 * @brief  Contains ComponentRegistry class used to have register of posible to create components..
 * @author Eryk Roszkowski
 ***********************************************/

#pragma once
#include "components/GameComponents/BasicComponents.hpp"
#include "components/errorUtils.hpp"
#include "components/pathUtils.hpp"
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
namespace vex
{
    ImTextureID GetEditorIcon_Internal(const std::string &name);
}

    template <typename T>
    void DrawAssetSlot(const char *label, T &value, ImTextureID icon, const char *hint)
    {
        ImGui::PushID(label);

        float thumbnailSize = 32.0f;
        std::string ext = std::filesystem::path(value.c_str()).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool good = true;
        if constexpr (std::is_same_v<T, vex::mesh_asset_path>)
        {
            good = vex::AssetExtensions::IsValid(ext, vex::AssetExtensions::Mesh);
        }
        else if constexpr (std::is_same_v<T, vex::texture_asset_path>)
        {
            good = vex::AssetExtensions::IsValid(ext, vex::AssetExtensions::Texture);
        }
        else if constexpr (std::is_same_v<T, vex::audio_asset_path>)
        {
            good = vex::AssetExtensions::IsValid(ext, vex::AssetExtensions::Audio);
        }

        ImTextureID fileIcon = vex::GetEditorIcon_Internal("file");
        ImTextureID drawIcon = (value.empty()) ? fileIcon : icon;
        if (drawIcon == 0)
        {
            drawIcon = fileIcon;
        }

        ImGui::BeginGroup();

        if (ImGui::ImageButton("##AssetIcon", drawIcon, {thumbnailSize, thumbnailSize}, ImVec2(0, 0), ImVec2(1, 1)))
        {
            /// @todo Open a list off all compatible assets (just like unreal does it)
        }

        ImGui::SameLine();
        ImGui::BeginGroup();

        static ImGuiID activeAssetID = 0;
        ImGuiID currentID = ImGui::GetID("##PathInput");

        if (activeAssetID == currentID)
        {
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
            {
                ImGui::SetKeyboardFocusHere(0);
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

            if (ImGui::InputText("##PathInput", (std::string *)&value, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                activeAssetID = 0;
            }

            if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
            {
                activeAssetID = 0;
            }
        }
        else
        {
            static std::unordered_map<std::string, std::pair<bool, double>> fileCache;
            bool fileExists = true;

            if (!value.empty())
            {
                double currentTime = ImGui::GetTime();
                auto &entry = fileCache[value];

                if (currentTime - entry.second > 5.0f)
                {
                    entry.first = std::filesystem::exists(vex::GetAssetPath(value));
                    entry.second = currentTime;
                }
                fileExists = entry.first;
            }

            std::string displayStr;
            bool colorPushed = false;
            if (value.empty())
            {
                displayStr = "Empty (" + std::string(hint) + ")";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                colorPushed = true;
            }
            else
            {
                displayStr = std::filesystem::path(value.c_str()).filename().string();
                if (!fileExists || !good)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
                    colorPushed = true;
                }
            }

            if (ImGui::Selectable(displayStr.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                activeAssetID = currentID;
            }

            if (colorPushed)
            {
                ImGui::PopStyleColor();
            }

            if (!value.empty() && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", value.c_str());
            }
        }
        ImGui::EndGroup();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.47f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.71f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.03f, 0.03f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.85f, 0.85f, 1.0f));

        if (ImGui::Button("Clear", ImVec2(0, 0)))
        {
            value.clear();
        }

        ImGui::PopStyleColor(4);
        ImGui::EndGroup();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ASSET_ITEM"))
            {
                std::string fullPath = (const char *)payload->Data;

                std::string ext = std::filesystem::path(fullPath).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                bool valid = true;

                if constexpr (std::is_same_v<T, vex::mesh_asset_path>)
                {
                    valid = (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb");
                }
                else if constexpr (std::is_same_v<T, vex::texture_asset_path>)
                {
                    valid = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp");
                }
                else if constexpr (std::is_same_v<T, vex::audio_asset_path>)
                {
                    valid = (ext == ".wav" || ext == ".ogg" || ext == ".flac");
                }

                if (valid)
                {
                    value = std::filesystem::relative(fullPath, vex::GetAssetDir()).string();
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char *label, vex::mesh_asset_path &value, ImSettings &settings, ImResponse &response)
    {
        DrawAssetSlot(label, value, vex::GetEditorIcon_Internal("mesh"), "Mesh");
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char *label, vex::texture_asset_path &value, ImSettings &settings, ImResponse &response)
    {
        DrawAssetSlot(label, value, vex::GetEditorIcon_Internal("texture"), "Texture");
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char *label, vex::audio_asset_path &value, ImSettings &settings, ImResponse &response)
    {
        DrawAssetSlot(label, value, vex::GetEditorIcon_Internal("audio"), "Audio");
    }

    inline void tag_invoke(ImReflect::ImInput_t, const char *label, vex::asset_path &value, ImSettings &settings, ImResponse &response)
    {
        DrawAssetSlot(label, value, vex::GetEditorIcon_Internal("file"), "Asset");
    }

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

    static ComponentRegistry& getInstance();

    /// @brief Template function to register a component type with the system.
    /// @details Generates and stores lambdas for:
    /// - **Loading**: Deserializes JSON into the component (creates it if missing).
    /// - **Saving**: Serializes component to JSON.
    /// - **Inspection**: Registers `GenericComponentInspector<T>` which uses `ImGui` and `ImReflect` to draw UI.
    /// - **Checking**: Checks if an object has this component.
    /// - **Creation**: Adds the default-constructed component to an object.
    /// @param const std::string& name - The name of the component class.
    /// @param bool isDynamic - Whether this component is part of a hot-reloadable module.
    template<typename T>
    void registerComponent(const std::string& name, bool isDynamic = false) {
        loaders[name] = [](GameObject& obj, const nlohmann::json& j) {
            if (!obj.HasComponent<T>()) {
                 if constexpr (std::is_default_constructible_v<T>) {
                     obj.AddComponent<T>(T());
                 }
            }
            if (obj.HasComponent<T>()) {
                T& comp = obj.GetComponent<T>();
                j.get_to(comp);
            }
        };

        savers[name] = [](GameObject& obj) -> nlohmann::json {
            if (obj.HasComponent<T>()) {
                return obj.GetComponent<T>();
            }
            return nullptr;
        };

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

        if (isDynamic) {
            dynamicComponents.push_back(name);
        }
    }

    /// @brief Unregisters a component type and removes all associated callbacks.
    /// @details Erases entries from `loaders`, `savers`, `inspectors`, `checkers`, and `creators` maps, and removes the name from `registeredNames`.
    /// @param const std::string& name - The name of the component to unregister.
    void unregisterComponent(const std::string& name);

    /// @brief Clears all components registered as dynamic.
    /// @details Used for hot-reloading. Iterates `dynamicComponents` and manually unregisters each one to clean up function pointers from unloaded DLLs.
    void clearDynamicComponents();

    /// @brief Loads data into a component on a GameObject from JSON.
    /// @details Finds the registered loader for `type` and executes it. Logs an error if the type is not registered.
    /// @param GameObject& obj - The target object.
    /// @param const std::string& type - The component type name.
    /// @param const nlohmann::json& json - The JSON data to load.
    /// @return bool - True if successful, false if type not found.
    bool loadComponent(GameObject& obj, const std::string& type, const nlohmann::json& json);

    /// @brief Serializes a component on a GameObject to JSON.
    /// @param GameObject& obj - The source object.
    /// @param const std::string& type - The component type name.
    /// @return nlohmann::json - The serialized data, or nullptr if not found/registered.
    nlohmann::json saveComponent(GameObject& obj, const std::string& type);

    /// @brief Draws the ImGui inspector for all components on the object.
    /// @details Iterates through all registered inspectors and calls them on the object.
    /// @param GameObject& obj - The object to inspect.
    void drawInspectorForObject(GameObject& obj);

    /// @brief Creates and attaches a specific component to a game object.
    /// @details Uses the registered creator lambda to default-construct and add the component.
    /// @param GameObject& obj - The target object.
    /// @param const std::string& name - The component type name.
    void createComponent(GameObject& obj, const std::string& name);

    /// @brief Get missing components for a game object.
    std::vector<std::string> getMissingComponents(const GameObject& obj);

    /// @brief Get component inspectors.
    const std::unordered_map<std::string, ComponentInspector>& getInspectors() const;

    /// @brief Get registered component names.
    const std::vector<std::string>& getRegisteredNames() const;

    #if DEBUG
        void SetEditorIcon(const std::string &name, ImTextureID icon);// { m_editorIcons[name] = icon; }
        ImTextureID GetEditorIcon(const std::string &name);// { return m_editorIcons[name]; }
    #endif

private:
    ComponentRegistry() = default;
    std::vector<std::string> registeredNames;
    std::vector<std::string> dynamicComponents;
    std::unordered_map<std::string, ComponentLoader> loaders;
    std::unordered_map<std::string, ComponentSaver> savers;
    std::unordered_map<std::string, ComponentInspector> inspectors;
    std::unordered_map<std::string, ComponentChecker> checkers;
    std::unordered_map<std::string, ComponentCreator> creators;
    #if DEBUG
        std::unordered_map<std::string, ImTextureID> m_editorIcons;
    #endif
};

}

/// @brief Helper macros to generate unique names based on line number
#define VEX_CAT_IMPL(a, b) a##b
#define VEX_CAT(a, b) VEX_CAT_IMPL(a, b)
#define VEX_UNIQUE_NAME(prefix) VEX_CAT(prefix, __LINE__)

#ifdef GAME_MODULE_EXPORTS
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
                    vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type, true); \
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
                    vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type, true); \
                } \
            }; \
            static VEX_UNIQUE_NAME(Registrar_) VEX_UNIQUE_NAME(g_registrar_); \
        }

#else
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
                    vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type, false); \
                } \
                ~VEX_UNIQUE_NAME(Registrar_)() { \
                    vex::ComponentRegistry::getInstance().unregisterComponent(#Type); \
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
                    vex::ComponentRegistry::getInstance().registerComponent<Type>(#Type, false); \
                } \
                ~VEX_UNIQUE_NAME(Registrar_)() { \
                    vex::ComponentRegistry::getInstance().unregisterComponent(#Type); \
                } \
            }; \
            static VEX_UNIQUE_NAME(Registrar_) VEX_UNIQUE_NAME(g_registrar_); \
        }
#endif
