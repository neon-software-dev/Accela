/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererSyncSystem.h"

#include "../Metrics.h"

#include "../Component/LightRenderableStateComponent.h"
#include "../Scene/ModelResources.h"
#include "../Scene/WorldState.h"
#include "../Model/ModelView.h"

#include <Accela/Engine/Component/ObjectRenderableComponent.h>
#include <Accela/Engine/Component/TerrainRenderableComponent.h>
#include <Accela/Engine/Component/LightComponent.h>

#include <Accela/Common/Timer.h>

namespace Accela::Engine
{

RendererSyncSystem::RendererSyncSystem(Common::ILogger::Ptr logger,
                                       Common::IMetrics::Ptr metrics,
                                       IWorldResources::Ptr worldResources,
                                       Render::IRenderer::Ptr renderer)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_worldResources(std::move(worldResources))
    , m_renderer(std::move(renderer))
{

}

void RendererSyncSystem::Initialize(entt::registry& registry)
{
    //
    // Observers for entities which have had enough components attached to them to now have renderable state
    //
    m_spriteCompletedObserver.connect(registry, entt::basic_collector<>::group<SpriteRenderableComponent, TransformComponent>());
    m_objectCompletedObserver.connect(registry, entt::basic_collector<>::group<ObjectRenderableComponent, TransformComponent>());
    m_modelCompletedObserver.connect(registry, entt::basic_collector<>::group<ModelRenderableComponent, ModelRenderableStateComponent, TransformComponent>());
    m_lightCompletedObserver.connect(registry, entt::basic_collector<>::group<LightComponent, TransformComponent>());

    //
    // Observers for entities with renderable state which has been updated
    //
    m_renderableStateUpdateObserver.connect(registry, entt::basic_collector<>::update<RenderableStateComponent>());
    m_lightStateUpdateObserver.connect(registry, entt::basic_collector<>::update<LightRenderableStateComponent>());

    //
    // Listeners for entities which have had renderable state removed
    //
    registry.on_destroy<RenderableStateComponent>().connect<&RendererSyncSystem::OnRenderableStateDestroyed>(this);
    registry.on_destroy<LightRenderableStateComponent>().connect<&RendererSyncSystem::OnLightRenderableStateDestroyed>(this);
}

void RendererSyncSystem::Execute(const RunState::Ptr& runState, entt::registry& registry)
{
    Common::Timer syncSystemTimer(Engine_RendererSyncSystem_Time);

    Render::WorldUpdate update{};

    ProcessNewlyCompletedRenderables(runState, registry, update);
    ProcessUpdatedRenderables(runState, registry, update);
    ProcessRenderablesToDestroy(runState, registry, update);

    if (update.HasAnyUpdate())
    {
        m_renderer->UpdateWorld(update);
    }

    syncSystemTimer.StopTimer(m_metrics);
}

void RendererSyncSystem::ProcessNewlyCompletedRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update)
{
    const auto virtualToRenderRatio = GetVirtualToRenderRatio(runState);

    //
    // Renderables
    //
    for (const auto& entity : m_spriteCompletedObserver)
    {
        CompleteSpriteRenderable(registry, entity, update, virtualToRenderRatio);
    }
    m_spriteCompletedObserver.clear();

    for (const auto& entity : m_objectCompletedObserver)
    {
        CompleteObjectRenderable(registry, entity, update);
    }
    m_objectCompletedObserver.clear();

    for (const auto& entity : m_modelCompletedObserver)
    {
        CompleteModelRenderable(registry, entity, update);
    }
    m_modelCompletedObserver.clear();

    //
    // Lights
    //
    for (const auto& entity : m_lightCompletedObserver)
    {
        CompleteLightRenderable(registry, entity, update);
    }
    m_lightCompletedObserver.clear();
}

void RendererSyncSystem::CompleteSpriteRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update, const glm::vec3& virtualToRenderRatio)
{
    // Create Renderable State
    auto& stateComponent = registry.emplace<RenderableStateComponent>(
        entity, RenderableStateComponent::Type::Sprite, registry.get<SpriteRenderableComponent>(entity).sceneName
    );

    // Create Renderable
    auto renderable = GetSpriteRenderable(registry, entity, virtualToRenderRatio);
    renderable.spriteId = m_renderer->GetIds()->spriteIds.GetId();

    // Record side effects
    stateComponent.renderableIds[0] = Render::RenderableId(renderable.spriteId.id);
    update.toAddSpriteRenderables.push_back(renderable);
}

void RendererSyncSystem::CompleteObjectRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update)
{
    // Create Renderable State
    auto& stateComponent = registry.emplace<RenderableStateComponent>(
        entity, RenderableStateComponent::Type::Object, registry.get<ObjectRenderableComponent>(entity).sceneName
    );

    // Create Renderable
    auto renderable = GetObjectRenderable(registry, entity);
    renderable.objectId = m_renderer->GetIds()->objectIds.GetId();

    // Record side effects
    stateComponent.renderableIds[0] = Render::RenderableId(renderable.objectId.id);
    update.toAddObjectRenderables.push_back(renderable);
    m_objectsToEntities.insert({renderable.objectId, entity});
}

void RendererSyncSystem::CompleteModelRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update)
{
    // Create Renderable State
    auto& stateComponent = registry.emplace<RenderableStateComponent>(
        entity, RenderableStateComponent::Type::Model, registry.get<ModelRenderableComponent>(entity).sceneName
    );

    // Create Renderable
    auto [modelComponent, animationComponent, transformComponent]
        = registry.get<ModelRenderableComponent, ModelRenderableStateComponent, TransformComponent>(entity);

    //
    // Pose the model according to the model's animation state. Will either return the model's bind pose
    // if no animation is active, or the proper pose for the animation if one exists
    //
    animationComponent.modelPose = GetModelPose(modelComponent.modelResource, modelComponent.animationState);
    if (!animationComponent.modelPose)
    {
        return;
    }

    auto renderables = GetModelRenderables(stateComponent, modelComponent, animationComponent, transformComponent);
    for (auto& it : renderables)
    {
        it.second.objectId = m_renderer->GetIds()->objectIds.GetId();

        // Record side effects
        stateComponent.renderableIds[it.first] = Render::RenderableId(it.second.objectId.id);
        update.toAddObjectRenderables.push_back(it.second);
        m_objectsToEntities.insert({it.second.objectId, entity});
    }
}

void RendererSyncSystem::CompleteLightRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update)
{
    // Create Renderable State
    auto& stateComponent = registry.emplace<LightRenderableStateComponent>(
        entity, registry.get<LightComponent>(entity).sceneName
    );

    // Create Renderable
    auto renderable = GetLightRenderable(registry, entity);
    renderable.lightId = m_renderer->GetIds()->lightIds.GetId();

    // Record side effects
    stateComponent.lightId = renderable.lightId;
    update.toAddLights.push_back(renderable);
}

void RendererSyncSystem::ProcessUpdatedRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update)
{
    const auto virtualToRenderRatio = GetVirtualToRenderRatio(runState);

    //
    // Renderables
    //
    for (const auto& entity : m_renderableStateUpdateObserver)
    {
        auto& stateComponent = registry.get<RenderableStateComponent>(entity);

        // Sanity check, shouldn't ever be the case
        if (stateComponent.state != ComponentState::Dirty)
        {
            assert(false);
            continue;
        }

        switch (stateComponent.type)
        {
            case RenderableStateComponent::Type::Sprite:
            {
                update.toUpdateSpriteRenderables.push_back(GetSpriteRenderable(registry, entity, virtualToRenderRatio));
            }
            break;

            case RenderableStateComponent::Type::Object:
            {
                update.toUpdateObjectRenderables.push_back(GetObjectRenderable(registry, entity));
            }
            break;

            case RenderableStateComponent::Type::Model:
            {
                auto [modelComponent, modelStateComponent, transformComponent] =
                    registry.get<ModelRenderableComponent, ModelRenderableStateComponent, TransformComponent>(entity);

                // Calculate the current model pose from the animation state
                modelStateComponent.modelPose = GetModelPose(modelComponent.modelResource, modelComponent.animationState);
                if (!modelStateComponent.modelPose)
                {
                    return;
                }

                const auto modelChanged = modelComponent.modelResource != modelStateComponent.modelResource;

                // If the entity is using the same model (usually the case), update its renderables from
                // the latest pose data
                if (!modelChanged)
                {
                    auto renderables = GetModelRenderables(stateComponent, modelComponent, modelStateComponent, transformComponent);

                    for (auto& it: renderables)
                    {
                        update.toUpdateObjectRenderables.push_back(it.second);
                    }
                }
                //
                // Otherwise, special case handle model change
                //
                else
                {
                    // Destroy all previous renderables
                    for (const auto& renderableId: stateComponent.renderableIds)
                    {
                        m_objectRenderablesToDestroy.insert(renderableId.second);
                    }
                    stateComponent.renderableIds.clear();

                    // Create renderables for the new model (note: we do this only after destroying old renderables so
                    // that the new renderables created here don't get assigned renderable ids from the stale state).
                    auto renderables = GetModelRenderables(stateComponent, modelComponent, modelStateComponent,
                                                           transformComponent);

                    // Create all new renderables
                    for (auto& it: renderables)
                    {
                        it.second.objectId = m_renderer->GetIds()->objectIds.GetId();

                        // Record side effects
                        stateComponent.renderableIds[it.first] = Render::RenderableId(it.second.objectId.id);

                        update.toAddObjectRenderables.push_back(it.second);
                    }

                    // Finalize
                    modelStateComponent.modelResource = modelComponent.modelResource;
                }
            }
            break;
        }

        stateComponent.state = ComponentState::Synced;
    }

    m_renderableStateUpdateObserver.clear();

    //
    // Lights
    //
    for (const auto& entity : m_lightStateUpdateObserver)
    {
        auto& stateComponent = registry.get<LightRenderableStateComponent>(entity);

        // Sanity check, shouldn't ever be the case
        if (stateComponent.state != ComponentState::Dirty)
        {
            assert(false);
            continue;
        }

        update.toUpdateLights.push_back(GetLightRenderable(registry, entity));

        stateComponent.state = ComponentState::Synced;
    }

    m_lightStateUpdateObserver.clear();
}

void RendererSyncSystem::ProcessRenderablesToDestroy(const RunState::Ptr&, entt::registry&, Render::WorldUpdate& update)
{
    std::ranges::transform(m_spriteRenderablesToDestroy, std::back_inserter(update.toDeleteSpriteIds), [](const auto& renderableId){
        return Render::SpriteId(renderableId.id);
    });
    m_spriteRenderablesToDestroy.clear();

    std::ranges::transform(m_objectRenderablesToDestroy, std::back_inserter(update.toDeleteObjectIds), [this](const auto& renderableId){
        m_objectsToEntities.erase(Render::ObjectId(renderableId.id));
        return Render::ObjectId(renderableId.id);
    });
    m_objectRenderablesToDestroy.clear();

    std::ranges::transform(m_lightsToDestroy, std::back_inserter(update.toDeleteLightIds), [](const auto& renderableId){
        return Render::LightId(renderableId.id);
    });
    m_lightsToDestroy.clear();
}

void RendererSyncSystem::OnRenderableStateDestroyed(entt::registry& registry, entt::entity entity)
{
    const auto& stateComponent = registry.get<RenderableStateComponent>(entity);

    switch (stateComponent.type)
    {
        case RenderableStateComponent::Type::Sprite:
        {
            for (const auto& renderableId : stateComponent.renderableIds)
            {
                m_spriteRenderablesToDestroy.insert(renderableId.second);
            }
        }
        break;

        case RenderableStateComponent::Type::Object:
        case RenderableStateComponent::Type::Model:
        {
            for (const auto& renderableId : stateComponent.renderableIds)
            {
                m_objectRenderablesToDestroy.insert(renderableId.second);
            }
        }
        break;
    }
}

void RendererSyncSystem::OnLightRenderableStateDestroyed(entt::registry& registry, entt::entity entity)
{
    const auto& stateComponent = registry.get<LightRenderableStateComponent>(entity);

    m_lightsToDestroy.insert(stateComponent.lightId);
}

Render::SpriteRenderable RendererSyncSystem::GetSpriteRenderable(entt::registry& registry, entt::entity entity, const glm::vec3& virtualToRenderRatio)
{
    auto [stateComponent, spriteComponent, transformComponent]
        = registry.get<RenderableStateComponent, SpriteRenderableComponent, TransformComponent>(entity);

    std::optional<Render::FSize> dstSize;
    if (spriteComponent.dstVirtualSize)
    {
        dstSize = Render::FSize(
            spriteComponent.dstVirtualSize->w / virtualToRenderRatio.x,
            spriteComponent.dstVirtualSize->h / virtualToRenderRatio.y
        );
    }

    Render::SpriteRenderable renderable{};
    renderable.sceneName = stateComponent.sceneName;
    renderable.textureId = spriteComponent.textureId;
    renderable.srcPixelRect = spriteComponent.srcPixelRect;
    renderable.dstSize = dstSize;
    renderable.position = transformComponent.GetPosition() / virtualToRenderRatio;
    renderable.orientation = transformComponent.GetOrientation();
    renderable.scale = transformComponent.GetScale();

    if (!stateComponent.renderableIds.empty())
    {
        renderable.spriteId = Render::SpriteId(stateComponent.renderableIds.at(0).id);
    }

    return renderable;
}

Render::ObjectRenderable RendererSyncSystem::GetObjectRenderable(entt::registry& registry, entt::entity entity)
{
    auto [stateComponent, objectComponent, transformComponent]
        = registry.get<RenderableStateComponent, ObjectRenderableComponent, TransformComponent>(entity);

    Render::ObjectRenderable renderable{};
    renderable.sceneName = objectComponent.sceneName;
    renderable.meshId = objectComponent.meshId;
    renderable.materialId = objectComponent.materialId;
    renderable.modelTransform = transformComponent.GetTransformMatrix();
    renderable.shadowPass = objectComponent.shadowPass;

    if (!stateComponent.renderableIds.empty())
    {
        renderable.objectId = Render::ObjectId(stateComponent.renderableIds.at(0).id);
    }

    return renderable;
}

std::vector<std::pair<std::size_t, Render::ObjectRenderable>> RendererSyncSystem::GetModelRenderables(RenderableStateComponent& stateComponent,
                                                                                                      const ModelRenderableComponent& modelComponent,
                                                                                                      const ModelRenderableStateComponent& modelStateComponent,
                                                                                                      const TransformComponent& transformComponent)
{
    //
    // Pose the model according to the model's animation state. Will either return the model's bind pose
    // if no animation is active, or the proper pose for the animation if one exists
    //
    std::vector<std::pair<std::size_t, Render::ObjectRenderable>> results;

    // Get object renderables for each static (non-bone) mesh
    for (const auto& meshPoseData : modelStateComponent.modelPose->meshPoseDatas)
    {
        results.emplace_back(
            NodeMeshId::HashFunction{}(meshPoseData.id),
            GetModelRenderable(stateComponent, modelComponent, transformComponent, meshPoseData)
        );
    }

    // Get object renderables for each bone mesh
    for (const auto& boneMesh : modelStateComponent.modelPose->boneMeshes)
    {
        results.emplace_back(
            NodeMeshId::HashFunction{}(boneMesh.meshPoseData.id),
            GetModelRenderable(stateComponent, modelComponent, transformComponent, boneMesh)
        );
    }

    return results;
}

Render::ObjectRenderable RendererSyncSystem::GetModelRenderable(RenderableStateComponent& stateComponent,
                                                                const ModelRenderableComponent& modelComponent,
                                                                const TransformComponent& transformComponent,
                                                                const MeshPoseData& meshPoseData)
{
    Render::ObjectRenderable renderable{};
    renderable.sceneName = modelComponent.sceneName;
    renderable.meshId = meshPoseData.modelMesh.meshId;
    renderable.materialId = meshPoseData.modelMesh.meshMaterialId;
    renderable.modelTransform = transformComponent.GetTransformMatrix() * meshPoseData.nodeTransform;
    renderable.shadowPass = modelComponent.shadowPass;

    const auto stateId = stateComponent.renderableIds.find(NodeMeshId::HashFunction{}(meshPoseData.id));
    if (stateId != stateComponent.renderableIds.cend())
    {
        renderable.objectId = Render::ObjectId(stateId->second.id);
    }

    return renderable;
}

Render::ObjectRenderable RendererSyncSystem::GetModelRenderable(RenderableStateComponent& stateComponent,
                                                                const ModelRenderableComponent& modelComponent,
                                                                const TransformComponent& transformComponent,
                                                                const BoneMesh& boneMesh)
{
    Render::ObjectRenderable renderable{};
    renderable.sceneName = modelComponent.sceneName;
    renderable.meshId = boneMesh.meshPoseData.modelMesh.meshId;
    renderable.materialId = boneMesh.meshPoseData.modelMesh.meshMaterialId;
    renderable.modelTransform = transformComponent.GetTransformMatrix() * boneMesh.meshPoseData.nodeTransform;
    renderable.boneTransforms = boneMesh.boneTransforms;
    renderable.shadowPass = modelComponent.shadowPass;

    const auto stateId = stateComponent.renderableIds.find(NodeMeshId::HashFunction{}(boneMesh.meshPoseData.id));
    if (stateId != stateComponent.renderableIds.cend())
    {
        renderable.objectId = Render::ObjectId(stateId->second.id);
    }

    return renderable;
}

Render::Light RendererSyncSystem::GetLightRenderable(entt::registry& registry, entt::entity entity)
{
    auto [stateComponent, lightComponent, transformComponent]
        = registry.get<LightRenderableStateComponent, LightComponent, TransformComponent>(entity);

    return {
        stateComponent.lightId,
        stateComponent.sceneName,
        transformComponent.GetPosition(), // TODO: Light shouldn't have orientation, use transform orientation
        lightComponent.castsShadows,
        lightComponent.lightProperties
    };
}

std::optional<ModelPose> RendererSyncSystem::GetModelPose(const ResourceIdentifier& model,
                                                          const std::optional<ModelAnimationState>& animationState)
{
    const auto registeredModelOpt = std::dynamic_pointer_cast<ModelResources>(m_worldResources->Models())->GetLoadedModel(model);
    if (!registeredModelOpt)
    {
        m_logger->Log(Common::LogLevel::Error, "RendererSyncSystem::GetModelPose: No such model exists: {}", model.GetUniqueName());
        return std::nullopt;
    }

    const auto modelView = ModelView(*registeredModelOpt);

    if (animationState.has_value())
    {
        return modelView.AnimationPose(animationState->animationName, animationState->animationTime);
    }
    else
    {
        return modelView.BindPose();
    }
}

glm::vec3 RendererSyncSystem::GetVirtualToRenderRatio(const RunState::Ptr& runState)
{
    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);
    const auto renderSettings = worldState->GetRenderSettings();

    const glm::vec2 virtualRes = runState->worldState->GetVirtualResolution();
    const glm::vec2 renderRes{renderSettings.resolution.w, renderSettings.resolution.h};
    const glm::vec3 virtualToRenderRatio{virtualRes.x / renderRes.x, virtualRes.y / renderRes.y, 1.0f};

    return virtualToRenderRatio;

}

std::optional<entt::entity> RendererSyncSystem::GetObjectEntity(const Render::ObjectId& objectId) const
{
    const auto it = m_objectsToEntities.find(objectId);
    if (it == m_objectsToEntities.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
