#pragma once

#include "VulkanMesh.hpp"
#include "components/GameComponents/BasicComponents.hpp"
#include "components/GameObjects/ModelObject.hpp"
#include "components/Mesh.hpp"
#include "components/VirtualFileSystem.hpp"

#include "Engine.hpp"

#include <unordered_map>
#include <components/Types.hpp>
#include <vector>
#include <memory>
#include "components/ECS/ECS.hpp"
#include <string>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace vex {
    class MeshManager {
    public:
        /// @brief Constructor for MeshManager.
        /// @param VulkanContext& context
        /// @param std::unique_ptr<VulkanResources>& resources
        /// @param VirtualFileSystem* vfs
        MeshManager(VulkanContext& context, std::unique_ptr<VulkanResources>& resources, VirtualFileSystem* vfs);
        ~MeshManager();

        /// @brief Initialize the MeshManager.
        /// @param Engine* engine
        void init(Engine* engine){
            m_p_engine = engine;

            auto& registry = m_p_engine->getRegistry();
            registry.on_create<MeshComponent>([this](vex::Entity entity, MeshComponent& comp) {
                onMeshComponentConstruct(m_p_engine->getRegistry(), entity);
            });
            registry.on_destroy<MeshComponent>([this](vex::Entity entity, MeshComponent& comp) {
                onMeshComponentDestroy(m_p_engine->getRegistry(), entity);
            });
        }

        /// @brief Loads mesh from a file, creates vulkan mesh and returns a MeshComponent.
        /// @param const std::string& path
        /// @return MeshComponent
        MeshComponent loadMesh(const std::string& path);

        /// @brief Creates a model object from a mesh component, transform component, and parent entity.
        /// @param const std::string& name
        /// @param MeshComponent meshComponent
        /// @param TransformComponent transformComponent
        /// @param vex::Entity parent
        /// @return ModelObject*
        ModelObject* createModel(const std::string& name, MeshComponent meshComponent, TransformComponent transformComponent, vex::Entity parent);

        /// @brief Destroys a model object by name and mesh component.
        /// @param std::string& name
        /// @param MeshComponent meshComponent
        void destroyModel(std::string& name, MeshComponent meshComponent);

        /// @brief Registers a Vulkan mesh component.
        /// @param MeshComponent& meshComponent
        void registerVulkanMesh(MeshComponent& meshComponent);

        /// @brief Retrieves a Vulkan mesh by mesh component.
        /// @param MeshComponent& meshComponent
        /// @return std::unique_ptr<VulkanMesh>&
        std::unique_ptr<VulkanMesh>& getVulkanMeshByMesh(MeshComponent& meshComponent);

        /// @brief Loads meshes asynchronously from the given paths.
        /// @param const std::vector<MeshComponent*>& pendingComponents
        void loadMeshesAsync(const std::vector<MeshComponent*>& pendingComponents);

        /// @brief Checks if a mesh is loaded by path.
        /// @param const std::string& path
        /// @return bool
        bool isMeshLoaded(const std::string& path) const {
            return m_vulkanMeshes.find(path) != m_vulkanMeshes.end();
        }

        /// @brief Clears the state of the mesh manager, resetting model IDs and clearing the mesh map.
        void clearState();

    private:
        VulkanContext& m_r_context;
        Engine* m_p_engine = nullptr;
        VirtualFileSystem* m_vfs;
        std::unique_ptr<VulkanResources>& m_p_resources;
        vex_map<std::string, std::unique_ptr<VulkanMesh>> m_vulkanMeshes;
        std::vector<uint32_t> m_freeModelIds;
        uint32_t m_nextModelId = 0;

        std::unordered_map<uint32_t, std::string> m_installedPaths;
        std::unordered_map<std::string, std::pair<glm::vec3, float>> m_meshBoundsCache;

        /// @brief Internally handles the construction of a mesh component called by entt callbacks.
        void onMeshComponentConstruct(vex::Registry& registry, vex::Entity entity);

        /// @brief Internally handles the destruction of a mesh component called by entt callbacks.
        void onMeshComponentDestroy(vex::Registry& registry, vex::Entity entity);

        /// @brief Internally handles the release of a mesh reference called by entt callbacks.
        void releaseMeshReference(const std::string& path, MeshComponent& ownerComp);
    };
}
