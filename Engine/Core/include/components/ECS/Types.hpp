#pragma once

#include <cstddef>
#include <cstdint>

namespace vex {
    /// @brief Type alias representing a unique entity identifier in the ECS system.
    /// @details Entities are lightweight handles (32-bit unsigned integers) that identify
    /// distinct objects in the entity-component-system. Each entity can have multiple
    /// components attached to it.
    using Entity = uint32_t;

    /// @brief Special entity value indicating an invalid or null entity.
    /// @details Used as a sentinel value to represent the absence of a valid entity.
    constexpr Entity NULL_ENTITY = static_cast<Entity>(-1);

    /// @brief Abstract base class for component storage implementations.
    /// @details This interface defines the contract that all component storage classes
    /// must follow. It allows the Registry to manage different component types uniformly
    /// through polymorphism.
    class IComponentStorage {
    public:
        /// @brief Virtual destructor for proper cleanup of derived classes.
        virtual ~IComponentStorage() = default;

        /// @brief Called when an entity is destroyed to clean up its associated components.
        /// @param entity The entity being destroyed.
        /// @details Implementations should remove any components associated with the given
        /// entity and invoke any registered destroy callbacks before cleanup.
        virtual void entity_destroyed(Entity entity) = 0;
    };
}
