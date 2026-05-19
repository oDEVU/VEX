/// @file ECS.hpp
/// @brief Main header for the Entity Component System (ECS) framework.
/// @details This header aggregates all ECS-related classes and provides a unified
/// interface for entity-component-system functionality. Include this file to access
/// the complete ECS API.
///
/// @defgroup ECS Entity Component System
/// @brief A flexible and efficient entity-component-system implementation.
/// @details The ECS module provides:
/// - Entity creation and management with ID recycling
/// - Type-safe component storage with cache-friendly dense packing
/// - Efficient entity queries through Views
/// - Lifecycle callbacks (onCreate/onDestroy) for components
///
/// @example
/// @code
/// // Create registry and entities
/// vex::Registry registry;
/// vex::Entity entity = registry.create();
///
/// // Add components
/// auto& position = registry.add_or_replace<Position>(entity, 10.0f, 20.0f);
/// auto& velocity = registry.add_or_replace<Velocity>(entity, 1.0f, 2.0f);
///
/// // Query entities with multiple components
/// vex::View<Position, Velocity> view(registry);
/// view.each([](vex::Entity e, Position& pos, Velocity& vel) {
///     pos.x += vel.x;
///     pos.y += vel.y;
/// });
///
/// // Register lifecycle callbacks
/// registry.on_create<Position>([](vex::Entity e, Position& pos) {
///     std::cout << "Position created for entity " << e << std::endl;
/// });
///
/// // Clean up
/// registry.destroy(entity);
/// @endcode

#pragma once

#include "Types.hpp"
#include "ComponentStorage.hpp"
#include "Registry.hpp"
#include "View.hpp"
