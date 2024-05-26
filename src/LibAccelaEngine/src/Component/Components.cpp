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

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const SpriteRenderableComponent& component);
template void RemoveComponent<SpriteRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<SpriteRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const ObjectRenderableComponent& component);
template void RemoveComponent<ObjectRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<ObjectRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const ModelRenderableComponent& component);
template void RemoveComponent<ModelRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<ModelRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const TerrainRenderableComponent& component);
template void RemoveComponent<TerrainRenderableComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<TerrainRenderableComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const TransformComponent& component);
template void RemoveComponent<TransformComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<TransformComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const LightComponent& component);
template void RemoveComponent<LightComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<LightComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

template void AddOrUpdateComponent(const IWorldState::Ptr&, EntityId entityId, const PhysicsComponent& component);
template void RemoveComponent<PhysicsComponent>(const IWorldState::Ptr& worldState, EntityId entityId);
template std::optional<PhysicsComponent> GetComponent(const IWorldState::Ptr&, EntityId entityId);

}
