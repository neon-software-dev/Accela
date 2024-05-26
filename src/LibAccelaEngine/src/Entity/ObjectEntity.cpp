#include <Accela/Engine/Entity/ObjectEntity.h>
#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Component/Components.h>

namespace Accela::Engine
{

ObjectEntity::Params& ObjectEntity::Params::WithStaticMesh(const Render::MeshId& _meshId) { meshId = _meshId; return *this; }
ObjectEntity::Params& ObjectEntity::Params::WithMaterial(const Render::MaterialId& _materialId) { materialId = _materialId; return *this; }
ObjectEntity::Params& ObjectEntity::Params::WithPosition(const glm::vec3& _position) { position = _position; return *this; }
ObjectEntity::Params& ObjectEntity::Params::WithScale(const glm::vec3& _scale) { scale = _scale; return *this; }
ObjectEntity::Params& ObjectEntity::Params::WithOrientation(const glm::quat& _orientation) { orientation = _orientation; return *this; }
ObjectEntity::Params& ObjectEntity::Params::WithPhysics(const PhysicsComponent& _physics) { physics = _physics; return *this; }

ObjectEntity::UPtr ObjectEntity::Create(const std::shared_ptr<IEngineRuntime>& engine,
                                      const Params& params,
                                      const std::string& sceneName)
{
    return std::make_unique<ObjectEntity>(
        ConstructTag{},
        engine,
        engine->GetWorldState()->CreateEntity(),
        sceneName,
        params
    );
}

ObjectEntity::ObjectEntity(ObjectEntity::ConstructTag,
                         std::shared_ptr<IEngineRuntime> engine,
                         EntityId eid,
                         std::string sceneName,
                         Params params)
    : Entity(std::move(engine), std::move(sceneName))
    , m_eid(eid)
    , m_params(std::move(params))
{
    SyncAll();
}

ObjectEntity::~ObjectEntity()
{
    DestroyInternal();
}

void ObjectEntity::Destroy()
{
    DestroyInternal();
}

void ObjectEntity::DestroyInternal()
{
    if (m_eid.has_value())
    {
        m_engine->GetWorldState()->DestroyEntity(*m_eid);
        m_eid = std::nullopt;
    }

    m_params = std::nullopt;
}

void ObjectEntity::SyncAll()
{
    if (CanSyncObjectRenderableComponent()) { SyncObjectRenderableComponent(); }
    if (CanSyncTransformComponent()) { SyncTransformComponent(); }
    if (CanSyncPhysicsComponent()) { SyncPhysicsComponent(); }
}

bool ObjectEntity::CanSyncObjectRenderableComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->meshId) { return false; }
    if (!m_params->materialId) { return false; }

    return true;
}

void ObjectEntity::SyncObjectRenderableComponent()
{
    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = m_sceneName;
    objectRenderableComponent.meshId = *m_params->meshId;
    objectRenderableComponent.materialId = *m_params->materialId;
    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, objectRenderableComponent);
}

bool ObjectEntity::CanSyncTransformComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->position) { return false; }

    return true;
}

void ObjectEntity::SyncTransformComponent()
{
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(*m_params->position);
    if (m_params->scale) { transformComponent.SetScale(*m_params->scale); }
    if (m_params->orientation) { transformComponent.SetOrientation(*m_params->orientation); }
    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);
}

bool ObjectEntity::CanSyncPhysicsComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->physics) { return false; }

    return true;
}

void ObjectEntity::SyncPhysicsComponent()
{
    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, *m_params->physics);
}

}
