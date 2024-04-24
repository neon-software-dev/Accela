/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/ModelEntity.h>
#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Component/Components.h>

namespace Accela::Engine
{

ModelEntity::Params& ModelEntity::Params::WithModelName(const std::string& _modelName) { modelName = _modelName; return *this; }
ModelEntity::Params& ModelEntity::Params::WithPosition(const glm::vec3& _position) { position = _position; return *this; }
ModelEntity::Params& ModelEntity::Params::WithScale(const glm::vec2& _scale) { scale = _scale; return *this; }
ModelEntity::Params& ModelEntity::Params::WithOrientation(const glm::quat& _orientation) { orientation = _orientation; return *this; }
ModelEntity::Params& ModelEntity::Params::IncludedInShadowPass(bool _inShadowPass) { inShadowPass = _inShadowPass; return *this; }

ModelEntity::UPtr ModelEntity::Create(const std::shared_ptr<IEngineRuntime>& engine,
                                      const Params& params,
                                      const std::string& sceneName)
{
    return std::make_unique<ModelEntity>(
        ConstructTag{},
        engine,
        engine->GetWorldState()->CreateEntity(),
        sceneName,
        params
    );
}

ModelEntity::ModelEntity(ModelEntity::ConstructTag,
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

ModelEntity::~ModelEntity()
{
    DestroyInternal();
}

void ModelEntity::Destroy()
{
    DestroyInternal();
}

void ModelEntity::DestroyInternal()
{
    if (m_eid.has_value())
    {
        m_engine->GetWorldState()->DestroyEntity(*m_eid);
        m_eid = std::nullopt;
    }

    m_params = std::nullopt;
}

void ModelEntity::RunAnimation(const ModelAnimationState& animationState)
{
    if (!m_eid) { return; }

    m_animationState = animationState;

    if (CanSyncModelComponent()) { SyncModelComponent(); }
}

void ModelEntity::StopAnimation()
{
    if (!m_eid) { return; }

    m_animationState = std::nullopt;

    if (CanSyncModelComponent()) { SyncModelComponent(); }
}

void ModelEntity::SyncAll()
{
    if (CanSyncModelComponent()) { SyncModelComponent(); }
    if (CanSyncTransformComponent()) { SyncTransformComponent(); }
}

bool ModelEntity::CanSyncModelComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->modelName) { return false; }

    return true;
}

void ModelEntity::SyncModelComponent()
{
    auto modelRenderableComponent = Engine::ModelRenderableComponent{};
    modelRenderableComponent.sceneName = m_sceneName;
    modelRenderableComponent.modelName = *m_params->modelName;

    if (m_params->inShadowPass) { modelRenderableComponent.shadowPass = *m_params->inShadowPass; }
    if (m_animationState) { modelRenderableComponent.animationState = *m_animationState; }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, modelRenderableComponent);
}

bool ModelEntity::CanSyncTransformComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->position) { return false; }

    return true;
}

void ModelEntity::SyncTransformComponent()
{
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(*m_params->position);
    if (m_params->scale) { transformComponent.SetScale(glm::vec3(*m_params->scale, 1.0f)); }
    if (m_params->orientation) { transformComponent.SetOrientation(*m_params->orientation); }
    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);
}

}
