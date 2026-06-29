#pragma once
#include <vector>
#include <functional>
#include <cassert>
#include <cstddef>
#include "Types.hpp"

namespace vex {

/// @brief Template-based storage container for components of a specific type.
/// @details ComponentStorage manages all instances of a particular component type across
/// all entities that possess it. It uses a dense packing strategy (packed array) to
/// maintain cache-friendly memory layout while supporting efficient entity lookups.
/// @tparam T The component type to be stored.
/// @note This class maintains three parallel data structures:
/// - m_components: Dense array of actual component instances
/// - m_indexToEntity: Mapping from component index to entity ID
/// - m_entityToIndex: Mapping from entity ID to component index
template <typename T>
class ComponentStorage : public IComponentStorage {
private:
    /// @brief Dense array of component instances, cache-optimized for iteration.
    std::vector<T> m_components;
    /// @brief Maps component storage index to the entity that owns the component.
    std::vector<Entity> m_indexToEntity;
    /// @brief Maps entity ID to the component storage index, or -1 if not present.
    std::vector<size_t> m_entityToIndex;

    /// @brief Callbacks invoked when a component is created for an entity.
    std::vector<std::function<void(Entity, T&)>> m_onCreate;
    /// @brief Callbacks invoked when a component is destroyed for an entity.
    std::vector<std::function<void(Entity, T&)>> m_onDestroy;

public:
    /// @brief Checks if the given entity has a component of type T.
    /// @param entity The entity to check.
    /// @return True if the entity has the component, false otherwise.
    bool has(Entity entity) const {
        return entity < m_entityToIndex.size() && m_entityToIndex[entity] != static_cast<size_t>(-1);
    }

    /// @brief Adds or replaces a component for an entity.
    /// @tparam Args Parameter types forwarded to the component constructor.
    /// @param entity The entity to add/replace the component for.
    /// @param args Arguments forwarded to the component's constructor.
    /// @return Reference to the newly added or replaced component.
    /// @details If the entity already has the component, it is replaced in-place.
    /// If not, a new component is created and all onCreate callbacks are invoked.
    template<typename... Args>
    T& add_or_replace(Entity entity, Args&&... args) {
        if (entity >= m_entityToIndex.size()) {
            m_entityToIndex.resize(entity + 1, static_cast<size_t>(-1));
        }

        if (has(entity)) {
            m_components[m_entityToIndex[entity]] = T{std::forward<Args>(args)...};
            return m_components[m_entityToIndex[entity]];
        }

        size_t newIndex = m_components.size();
        m_entityToIndex[entity] = newIndex;
        m_indexToEntity.push_back(entity);
        m_components.emplace_back(std::forward<Args>(args)...);

        T& component = m_components.back();
        for (auto& cb : m_onCreate) cb(entity, component);

        return component;
    }

    /// @brief Removes the component from an entity.
    /// @param entity The entity to remove the component from.
    /// @details If the entity does not have the component, this operation is a no-op.
    /// All onDestroy callbacks are invoked before removal. Uses swap-and-pop to maintain
    /// dense packing without gaps in the storage array.
    void remove(Entity entity) {
        if (!has(entity)) return;

        size_t indexOfRemoved = m_entityToIndex[entity];
        size_t indexOfLast = m_components.size() - 1;

        for (auto& cb : m_onDestroy) cb(entity, m_components[indexOfRemoved]);

        if (indexOfRemoved != indexOfLast) {
            Entity entityOfLast = m_indexToEntity[indexOfLast];
            m_components[indexOfRemoved] = std::move(m_components[indexOfLast]);
            m_indexToEntity[indexOfRemoved] = entityOfLast;
            m_entityToIndex[entityOfLast] = indexOfRemoved;
        }

        m_components.pop_back();
        m_indexToEntity.pop_back();
        m_entityToIndex[entity] = static_cast<size_t>(-1);
    }

    /// @brief Retrieves a reference to the component for an entity.
    /// @param entity The entity whose component to retrieve.
    /// @return Mutable reference to the component.
    /// @pre The entity must have the component (checked via assert in debug builds).
    T& get(Entity entity) {
        assert(has(entity) && "Retrieving non-existent component.");
        return m_components[m_entityToIndex[entity]];
    }

    /// @brief Called by the Registry when an entity is destroyed.
    /// @param entity The entity being destroyed.
    /// @details Invokes the remove() method to clean up the component and invoke callbacks.
    void entity_destroyed(Entity entity) override {
        remove(entity);
    }

    /// @brief Registers a callback to be invoked when a component is created.
    /// @param cb The callback function to register.
    /// @details Callback is invoked with the entity and component reference.
    void on_create(std::function<void(Entity, T&)> cb) { m_onCreate.push_back(cb); }

    /// @brief Registers a callback to be invoked when a component is destroyed.
    /// @param cb The callback function to register.
    /// @details Callback is invoked with the entity and component reference.
    void on_destroy(std::function<void(Entity, T&)> cb) { m_onDestroy.push_back(cb); }

    /// @brief Returns the list of all entities that have this component type.
    /// @return Const reference to the vector of entities.
    const std::vector<Entity>& get_entities() const { return m_indexToEntity; }
};

}
