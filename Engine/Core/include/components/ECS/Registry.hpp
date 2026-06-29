#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "Types.hpp"
#include "ComponentStorage.hpp"
#include "VEX/VEX_export.h"

namespace vex {

/// @brief Manages unique type IDs for component types in the ECS system.
/// @details Uses a static counter to assign unique identifiers to each component type
/// at compile-time, enabling type-safe component management throughout the registry.
class VEX_EXPORT ComponentType {
public:
    static uint32_t get_id_by_name(const char* name);

    template <typename T>
    static uint32_t get_id() {
        static const uint32_t id = get_id_by_name(typeid(T).name());
        return id;
    }
};

/// @brief Central registry for managing entities and their components in the ECS system.
/// @details The Registry handles entity creation and destruction, component storage management,
/// and provides a unified interface for component access. It maintains a pool of available
/// entity IDs for reuse and dynamically creates storage for each component type as needed.
class Registry {
private:
    /// @brief Pool of reusable entity IDs from destroyed entities.
    std::vector<Entity> m_availableEntities;
    /// @brief Counter for generating new entity IDs when the pool is empty.
    Entity m_nextEntity = 0;
    /// @brief Array of component storages indexed by component type ID.
    std::vector<std::unique_ptr<IComponentStorage>> m_componentStorages;

    /// @brief Retrieves or creates the storage for component type T.
    /// @tparam T The component type.
    /// @return Pointer to the ComponentStorage for type T.
    /// @details Lazily initializes storage on first access and resizes the storage array
    /// as needed to accommodate new component types.
    template <typename T>
    ComponentStorage<T>* get_storage() {
        uint32_t typeId = ComponentType::get_id<T>();
        if (typeId >= m_componentStorages.size()) {
            m_componentStorages.resize(typeId + 1);
        }
        if (!m_componentStorages[typeId]) {
            m_componentStorages[typeId] = std::make_unique<ComponentStorage<T>>();
        }
        return static_cast<ComponentStorage<T>*>(m_componentStorages[typeId].get());
    }

public:
    /// @brief Attempts to retrieve the storage for component type T without creating it.
    /// @tparam T The component type.
    /// @return Pointer to the ComponentStorage if it exists, nullptr otherwise.
    /// @details Used internally to check if a component type has been stored before.
    template <typename T>
    ComponentStorage<T>* try_get_storage_internal() {
        uint32_t typeId = ComponentType::get_id<T>();
        if (typeId >= m_componentStorages.size() || !m_componentStorages[typeId]) {
            return nullptr;
        }
        return static_cast<ComponentStorage<T>*>(m_componentStorages[typeId].get());
    }

    /// @brief Creates a new entity.
    /// @return The ID of the newly created entity.
    /// @details Reuses entity IDs from destroyed entities if available, otherwise generates
    /// a new ID from the counter. This provides stable entity IDs even with entity recycling.
    Entity create() {
        if (!m_availableEntities.empty()) {
            Entity e = m_availableEntities.back();
            m_availableEntities.pop_back();
            return e;
        }
        return m_nextEntity++;
    }

    /// @brief Destroys an entity and all its associated components.
    /// @param entity The entity to destroy.
    /// @details Notifies all component storages of the entity destruction, triggering
    /// onDestroy callbacks and cleanup. The entity ID is returned to the pool for reuse.
    void destroy(Entity entity) {
        for (auto& Storage : m_componentStorages) {
            if (Storage) Storage->entity_destroyed(entity);
        }
        m_availableEntities.push_back(entity);
    }

    /// @brief Adds or replaces a component for an entity.
    /// @tparam T The component type.
    /// @tparam Args Parameter types for the component constructor.
    /// @param entity The entity to add/replace the component for.
    /// @param args Arguments forwarded to the component constructor.
    /// @return Reference to the newly added or replaced component.
    template<typename T, typename... Args>
    T& add_or_replace(Entity entity, Args&&... args) {
        return get_storage<T>()->add_or_replace(entity, std::forward<Args>(args)...);
    }

    /// @brief Removes a component from an entity.
    /// @tparam T The component type to remove.
    /// @param entity The entity to remove the component from.
    template<typename T>
    void remove(Entity entity) {
        get_storage<T>()->remove(entity);
    }

    /// @brief Retrieves a component from an entity.
    /// @tparam T The component type.
    /// @param entity The entity to get the component from.
    /// @return Mutable reference to the component.
    /// @pre The entity must have the component.
    template<typename T>
    T& get(Entity entity) {
        return get_storage<T>()->get(entity);
    }

    /// @brief Safely retrieves a component from an entity if it exists.
    /// @tparam T The component type.
    /// @param entity The entity to get the component from.
    /// @return Pointer to the component if it exists, nullptr otherwise.
    template<typename T>
    T* try_get(Entity entity) {
        auto* arr = try_get_storage_internal<T>();
        return (arr && arr->has(entity)) ? &arr->get(entity) : nullptr;
    }

    /// @brief Checks if an entity has a component of type T.
    /// @tparam T The component type to check for.
    /// @param entity The entity to check.
    /// @return True if the entity has the component, false otherwise.
    template<typename T>
    bool has(Entity entity) {
        auto* arr = try_get_storage_internal<T>();
        return arr != nullptr && arr->has(entity);
    }

    /// @brief Registers a callback to be invoked when a component of type T is created.
    /// @tparam T The component type.
    /// @param cb The callback function to register.
    template<typename T>
    void on_create(std::function<void(Entity, T&)> cb) { get_storage<T>()->on_create(cb); }

    /// @brief Registers a callback to be invoked when a component of type T is destroyed.
    /// @tparam T The component type.
    /// @param cb The callback function to register.
    template<typename T>
    void on_destroy(std::function<void(Entity, T&)> cb) { get_storage<T>()->on_destroy(cb); }
};

}
