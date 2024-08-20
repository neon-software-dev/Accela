/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_COMPONENTS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_COMPONENTS_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Scene/IWorldState.h>

#include <Accela/Engine/Component/SpriteRenderableComponent.h>
#include <Accela/Engine/Component/ObjectRenderableComponent.h>
#include <Accela/Engine/Component/ModelRenderableComponent.h>
#include <Accela/Engine/Component/TerrainRenderableComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
#include <Accela/Engine/Component/LightComponent.h>
#include <Accela/Engine/Component/PhysicsComponent.h>

#include <optional>

namespace Accela::Engine
{
    /**
     * Adds the provided component to the specified entity, or updates the entity's component, if it
     * already had one of the same type.
     *
     * @tparam T The component type
     * @param worldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be modified
     * @param component The component to be added/updated
     */
    template <typename T>
    void AddOrUpdateComponent(const IWorldState::Ptr& worldState, EntityId entityId, const T& component);

    /**
     * Removes a component from an entity
     *
     * @tparam T The component type
     * @param worldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be modified
     */
    template <typename T>
    void RemoveComponent(const IWorldState::Ptr& worldState, EntityId entityId);

    /**
     * Gets the current value of an entity's component
     *
     * @tparam T The component type to query
     * @param worldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be accessed
     *
     * @return A copy of the component's value, or std::nullopt if no such component
     */
    template <typename T>
    std::optional<T> GetComponent(const IWorldState::Ptr& worldState, EntityId entityId);
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_COMPONENTS_H
