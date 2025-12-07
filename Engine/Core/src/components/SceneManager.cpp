#include "components/SceneManager.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameComponents/ComponentFactory.hpp"
#include "components/GameObjects/GameObjectFactory.hpp"
#include "components/GameObjects/CameraObject.hpp"
#include "components/GameObjects/Creators/ModelCreator.hpp"
#include "components/PhysicsSystem.hpp"
#include "components/enviroment.hpp"
#include "VirtualFileSystem.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <filesystem>

namespace vex {

    void to_json(nlohmann::json& j, const TransformComponent& t) {
            j = nlohmann::json{
                {"position", t.getLocalPosition()},
                {"rotation", t.getLocalRotation()},
                {"scale",    t.getLocalScale()}
            };
        }

        void from_json(const nlohmann::json& j, TransformComponent& t) {
            if(j.contains("position")) t.setLocalPosition(j["position"]);
            if(j.contains("rotation")) t.setLocalRotation(j["rotation"]);
            if(j.contains("scale"))    t.setLocalScale(j["scale"]);
        }


        void to_json(nlohmann::json& j, const MeshComponent& m) {
                j["path"] = m.meshData.meshPath;
                j["renderType"] = (int)m.renderType;
                j["color"] = m.color;
            }

            void from_json(const nlohmann::json& j, MeshComponent& m) {
                if (j.contains("path")) {
                    std::string path = j["path"];
                    m.meshData.meshPath = path;
                }
                if (j.contains("renderType")) m.renderType = (RenderType)j["renderType"];
                if (j.contains("color")) m.color = j["color"];
            }


        //REGISTER_COMPONENT(MeshComponent, meshData, renderType, color);

        #if DEBUG
        template<>
            void vex::GenericComponentInspector<vex::PhysicsComponent>(GameObject& obj) {
                if (obj.HasComponent<vex::PhysicsComponent>()) {
                    if (ImGui::CollapsingHeader("PhysicsComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& pc = obj.GetComponent<vex::PhysicsComponent>();

                        ImReflect::Input("Shape", pc.shape); // Draw Enum

                        if (pc.shape == ShapeType::BOX)
                            ImGui::DragFloat3("Extents", &pc.boxHalfExtents.x);
                        else if (pc.shape == ShapeType::SPHERE)
                            ImGui::DragFloat("Radius", &pc.sphereRadius);
                        else if (pc.shape == ShapeType::CAPSULE)
                            ImGui::DragFloat2("Capsule", &pc.capsuleRadius);
                        else if (pc.shape == ShapeType::CYLINDER)
                            ImGui::DragFloat2("Cylinder", &pc.cylinderRadius);

                        ImReflect::Input("Mass", pc.mass);
                        ImReflect::Input("Friction", pc.friction);
                        ImReflect::Input("Bounciness", pc.bounce);
                        ImReflect::Input("Linear Damping", pc.linearDamping);
                        ImReflect::Input("Angular Damping", pc.angularDamping);
                        ImReflect::Input("Sensor", pc.isSensor);
                        ImReflect::Input("Allow Sleeping", pc.allowSleeping);

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.47f, 0.05f, 0.05f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.71f, 0.10f, 0.10f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.03f, 0.03f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.85f, 0.85f, 1.0f)); // White Text

                        if (ImGui::Button("Remove")) {
                            obj.GetEngine().getRegistry().remove<vex::PhysicsComponent>(obj.GetEntity());
                        }

                        ImGui::PopStyleColor(4);
                    }
                }
            }
            #endif
}


REGISTER_COMPONENT_CUSTOM(vex::TransformComponent, position, rotation, scale);
REGISTER_COMPONENT_CUSTOM(vex::MeshComponent, meshData, renderType, color);

REGISTER_COMPONENT(vex::CameraComponent, fov, nearPlane, farPlane);
REGISTER_COMPONENT(vex::LightComponent, color, intensity, radius);
//REGISTER_COMPONENT(vex::PhysicsComponent, shape, mass, friction, bounce, linearDamping, angularDamping, allowSleeping);

REGISTER_COMPONENT(vex::PhysicsComponent,
    shape,
    bodyType,
    isSensor,
    allowSleeping,

    mass,
    friction,
    bounce,
    linearDamping,
    angularDamping,

    boxHalfExtents,
    sphereRadius,
    capsuleRadius,
    capsuleHeight,
    cylinderRadius,
    cylinderHeight
);

namespace vex {

    REGISTER_GAME_OBJECT(CameraObject);
    REGISTER_GAME_OBJECT(GameObject);

void SceneManager::loadScene(const std::string& path, Engine& engine) {
    clearScenes();
    loadSceneWithoutClearing(path, engine);
}

void SceneManager::unloadScene(const std::string& path) {
    m_scenes.erase(path);
}

void SceneManager::loadSceneWithoutClearing(const std::string& path, Engine& engine) {
    lastSceneName = path;
    m_scenes.emplace(path, std::make_shared<Scene>(path, engine));
    m_scenes[path]->sceneBegin();
}

void SceneManager::clearScenes() {
    m_scenes.clear();
}

void SceneManager::scenesUpdate(float deltaTime){
    for (auto& scene : m_scenes) {
        scene.second->sceneUpdate(deltaTime);
    }
}

std::vector<std::string> SceneManager::GetAllSceneNames() const {
    std::vector<std::string> names;
    names.reserve(m_scenes.size());

    for (const auto& [name, scene] : m_scenes) {
        names.push_back(name);
    }

    return names;
}
}
