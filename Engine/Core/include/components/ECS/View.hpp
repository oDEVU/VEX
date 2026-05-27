#pragma once
#include <tuple>
#include "Types.hpp"
#include "Registry.hpp"

namespace vex {

/// @brief Provides iteration over entities that have all specified component types.
/// @details View is a query interface that allows efficient iteration over all entities
/// matching a given component signature. It uses the first component type as the primary
/// storage to iterate over, then filters based on presence of other components.
/// @tparam Components Variable number of component types that entities must have.
/// @note This is a lightweight, non-owning view into the Registry's component data.
/// The Registry must remain valid for the lifetime of the View.

template<typename... Components>
class View {
private:
    /// @brief Reference to the Registry containing the component data.
    Registry& m_registry;

public:
    /// @brief Constructs a View for querying entities with the specified component types.
    /// @param reg Reference to the Registry to query.
    View(Registry& reg) : m_registry(reg) {}

    /// @brief Iterates over all entities with the specified component types.
    /// @tparam Func The callable type that processes each matching entity.
    /// @param func Callback function invoked for each matching entity.
    /// @details The callback receives the entity ID followed by references to all components
    /// in the order specified in the View template parameters. Example usage:
    /// @code
    /// View<Position, Velocity> view(registry);
    /// view.each([](Entity e, Position& pos, Velocity& vel) {
    ///     pos.x += vel.x;
    ///     pos.y += vel.y;
    /// });
    /// @endcode
    template<typename Func>
    void each(Func func) {
        using FirstT = typename std::tuple_element<0, std::tuple<Components...>>::type;

        auto* leadArray = m_registry.try_get_storage_internal<FirstT>();
        if (!leadArray) return;

        auto& entities = leadArray->get_entities();

        for (ptrdiff_t i = static_cast<ptrdiff_t>(entities.size()) - 1; i >= 0; --i) {

            if (i >= static_cast<ptrdiff_t>(entities.size())) continue;

            Entity e = entities[i];

            if ((m_registry.has<Components>(e) && ...)) {
                func(e, m_registry.get<Components>(e)...);
            }
        }
    }
};

}
