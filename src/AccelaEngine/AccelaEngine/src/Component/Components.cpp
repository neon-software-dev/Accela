/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Component/Components.h>

#include "../Scene/WorldState.h"

namespace Accela::Engine
{

template <typename T>
void AddOrUpdateComponent(const IWorldState::Ptr& worldState, EntityId entityId, const T& component)
{
    std::static_pointer_cast<WorldState>(worldState)->AddOrUpdateComponent(entityId, component);
}

template <typename T>
void RemoveComponent(const IWorldState::Ptr& worldState, EntityId entityId)
{
    std::static_pointer_cast<WorldState>(worldState)->RemoveComponent<T>(entityId);
}

template <typename T>
std::optional<T> GetComponent(const IWorldState::Ptr& worldState, EntityId entityId)
{
    return std::static_pointer_cast<WorldState>(worldState)->GetComponent<T>(entityId);
}

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const SpriteRenderableComponent& component);
template ACCELA_PUBLIC void RemoveComponent<SpriteRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<SpriteRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const ObjectRenderableComponent& component);
template ACCELA_PUBLIC void RemoveComponent<ObjectRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<ObjectRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const ModelRenderableComponent& component);
template ACCELA_PUBLIC void RemoveComponent<ModelRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<ModelRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const TerrainRenderableComponent& component);
template ACCELA_PUBLIC void RemoveComponent<TerrainRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<TerrainRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const TransformComponent& component);
template ACCELA_PUBLIC void RemoveComponent<TransformComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<TransformComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const LightComponent& component);
template ACCELA_PUBLIC void RemoveComponent<LightComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<LightComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template ACCELA_PUBLIC void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const PhysicsComponent& component);
template ACCELA_PUBLIC void RemoveComponent<PhysicsComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template ACCELA_PUBLIC std::optional<PhysicsComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

}
