/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererSyncSystem.h"

#include "../Metrics.h"

#include "../Component/RenderableStateComponent.h"
#include "../Component/LightRenderableStateComponent.h"
#include "../Component/ModelAnimationComponent.h"
#include "../Scene/WorldResources.h"
#include "../Scene/WorldState.h"
#include "../Model/ModelView.h"

#include <Accela/Engine/Component/SpriteRenderableComponent.h>
#include <Accela/Engine/Component/ObjectRenderableComponent.h>
#include <Accela/Engine/Component/TerrainRenderableComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>
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

void RendererSyncSystem::Execute(const RunState::Ptr& runState, entt::registry& registry)
{
    Common::Timer syncSystemTimer(Engine_RendererSyncSystem_Time);

    Render::WorldUpdate update{};

    SyncSpriteRenderables(runState, registry, update);
    SyncObjectRenderables(runState, registry, update);
    SyncModelRenderables(runState, registry, update);
    SyncTerrainRenderables(runState, registry, update);
    SyncLights(runState, registry, update);

    if (update.HasAnyUpdate())
    {
        m_renderer->UpdateWorld(update);
    }

    syncSystemTimer.StopTimer(m_metrics);
}

void RendererSyncSystem::SyncSpriteRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update)
{
    const auto worldState = std::dynamic_pointer_cast<WorldState>(runState->worldState);
    const auto renderSettings = worldState->GetRenderSettings();

    const glm::vec2 virtualRes = runState->worldState->GetVirtualResolution();
    const glm::vec2 renderRes{renderSettings.resolution.w, renderSettings.resolution.h};
    const glm::vec3 virtualToRenderRatio{virtualRes.x / renderRes.x, virtualRes.y / renderRes.y, 1.0f};

    //
    // Added and updated sprite renderables
    //
    registry.view<RenderableStateComponent, SpriteRenderableComponent, TransformComponent>().each(
    [&](const auto&,
            RenderableStateComponent& renderableComponent,
            SpriteRenderableComponent& spriteRenderableComponent,
            const TransformComponent& transformComponent)
    {
        std::optional<Render::FSize> dstSize;
        if (spriteRenderableComponent.dstVirtualSize)
        {
            dstSize = Render::FSize(
                spriteRenderableComponent.dstVirtualSize->w / virtualToRenderRatio.x,
                spriteRenderableComponent.dstVirtualSize->h / virtualToRenderRatio.y
            );
        }

        Render::SpriteRenderable spriteRenderable{};
        spriteRenderable.spriteId = Render::SpriteId(renderableComponent.renderableIds[0].id);
        spriteRenderable.sceneName = renderableComponent.sceneName;
        spriteRenderable.textureId = spriteRenderableComponent.textureId;
        spriteRenderable.srcPixelRect = spriteRenderableComponent.srcPixelRect;
        spriteRenderable.dstSize = dstSize;
        spriteRenderable.position = transformComponent.GetPosition() / virtualToRenderRatio;
        spriteRenderable.orientation = transformComponent.GetOrientation();
        spriteRenderable.scale = transformComponent.GetScale();

        switch (renderableComponent.state)
        {
            case ComponentState::New:
            {
                spriteRenderable.spriteId = m_renderer->GetIds()->spriteIds.GetId();
                renderableComponent.renderableIds[0] = Render::RenderableId(spriteRenderable.spriteId.id);

                update.toAddSpriteRenderables.push_back(spriteRenderable);
            }
            break;
            case ComponentState::Dirty:
            {
                update.toUpdateSpriteRenderables.push_back(spriteRenderable);
            }
            break;
            case ComponentState::Synced:
                // no-op TODO Perf:: Don't include synced items in the loop
            break;
        }

        renderableComponent.state = ComponentState::Synced;
    });

    //
    // Deleted sprite renderables
    //
    for (const auto& it : m_destroyedSpriteRenderables)
    {
        for (const auto& renderableId : it.second)
        {
            update.toDeleteSpriteIds.emplace_back(renderableId.id);
        }
    }
    m_destroyedSpriteRenderables.clear();
}

void RendererSyncSystem::SyncObjectRenderables(const RunState::Ptr&, entt::registry& registry, Render::WorldUpdate& update)
{
    registry.view<RenderableStateComponent, ObjectRenderableComponent, TransformComponent>().each(
        [&](const auto&,
            RenderableStateComponent& renderableComponent,
            ObjectRenderableComponent& objectRenderableComponent,
            const TransformComponent& transformComponent)
        {
            Render::ObjectRenderable objectRenderable{};
            objectRenderable.sceneName = renderableComponent.sceneName;
            objectRenderable.meshId = objectRenderableComponent.meshId;
            objectRenderable.materialId = objectRenderableComponent.materialId;
            objectRenderable.modelTransform = transformComponent.GetTransformMatrix();
            objectRenderable.shadowPass = objectRenderableComponent.shadowPass;

            switch (renderableComponent.state)
            {
                case ComponentState::New:
                {
                    objectRenderable.objectId = m_renderer->GetIds()->objectIds.GetId();
                    renderableComponent.renderableIds[0] = Render::RenderableId(objectRenderable.objectId.id);

                    update.toAddObjectRenderables.push_back(objectRenderable);
                }
                break;
                case ComponentState::Dirty:
                {
                    objectRenderable.objectId = Render::ObjectId(renderableComponent.renderableIds[0].id);

                    update.toUpdateObjectRenderables.push_back(objectRenderable);
                }
                break;
                case ComponentState::Synced:
                    // no-op TODO Perf:: Don't include synced items in the loop
                break;
            }

            renderableComponent.state = ComponentState::Synced;
        });

    //
    // Deleted object renderables
    //
    for (const auto& it : m_destroyedObjectRenderables)
    {
        for (const auto& renderableId : it.second)
        {
            update.toDeleteObjectIds.emplace_back(renderableId.id);
        }
    }
    m_destroyedObjectRenderables.clear();
}

void RendererSyncSystem::SyncModelRenderables(const RunState::Ptr&, entt::registry& registry, Render::WorldUpdate& update)
{
    registry.view<RenderableStateComponent, ModelRenderableComponent, ModelAnimationComponent, TransformComponent>().each(
        [&](const auto&,
            RenderableStateComponent& renderableComponent,
            ModelRenderableComponent& modelRenderableComponent,
            ModelAnimationComponent& modelAnimationComponent,
            const TransformComponent& transformComponent)
        {
            //
            // Post the model according to the model's animation state. Will either return the model's bind pose
            // if no animation is active, or the proper pose for the animation if one exists
            //
            modelAnimationComponent.modelPose = GetModelPose(modelRenderableComponent.modelName, modelRenderableComponent.animationState);
            if (!modelAnimationComponent.modelPose)
            {
                return;
            }

            //
            // Create/update/delete renderables for the entity/model to display the model in its pose
            //
            switch (renderableComponent.state)
            {
                case ComponentState::New:
                {
                    std::ranges::transform(modelAnimationComponent.modelPose->meshPoseDatas,
                                           std::back_inserter(update.toAddObjectRenderables),
                                           [&,this](const auto& meshPoseData){
                       Render::ObjectRenderable objectRenderable{};
                       objectRenderable.objectId = m_renderer->GetIds()->objectIds.GetId();
                       objectRenderable.sceneName = renderableComponent.sceneName;
                       objectRenderable.meshId = meshPoseData.modelMesh.meshId;
                       objectRenderable.materialId = meshPoseData.modelMesh.meshMaterialId;
                       objectRenderable.modelTransform = transformComponent.GetTransformMatrix() * meshPoseData.nodeTransform;
                       objectRenderable.shadowPass = modelRenderableComponent.shadowPass;

                       renderableComponent.renderableIds[NodeMeshId::HashFunction{}(meshPoseData.id)]
                            = Render::RenderableId(objectRenderable.objectId.id);

                       return objectRenderable;
                    });

                    std::ranges::transform(modelAnimationComponent.modelPose->boneMeshes,
                                           std::back_inserter(update.toAddObjectRenderables),
                                           [&,this](const auto& boneMesh){
                       Render::ObjectRenderable objectRenderable{};
                       objectRenderable.objectId = m_renderer->GetIds()->objectIds.GetId();
                       objectRenderable.sceneName = renderableComponent.sceneName;
                       objectRenderable.meshId = boneMesh.meshPoseData.modelMesh.meshId;
                       objectRenderable.materialId = boneMesh.meshPoseData.modelMesh.meshMaterialId;
                       objectRenderable.modelTransform = transformComponent.GetTransformMatrix() * boneMesh.meshPoseData.nodeTransform;
                       objectRenderable.boneTransforms = boneMesh.boneTransforms;
                       objectRenderable.shadowPass = modelRenderableComponent.shadowPass;

                       renderableComponent.renderableIds[NodeMeshId::HashFunction{}(boneMesh.meshPoseData.id)]
                           = Render::RenderableId(objectRenderable.objectId.id);

                       return objectRenderable;
                   });
                }
                break;
                case ComponentState::Dirty:
                {
                    std::ranges::transform(modelAnimationComponent.modelPose->meshPoseDatas,
                                           std::back_inserter(update.toUpdateObjectRenderables),
                                           [&](const auto& meshPoseData){
                       const Render::RenderableId renderableId = renderableComponent.renderableIds[NodeMeshId::HashFunction{}(meshPoseData.id)];

                       Render::ObjectRenderable objectRenderable{};
                       objectRenderable.objectId = Render::ObjectId(renderableId.id);
                       objectRenderable.sceneName = renderableComponent.sceneName;
                       objectRenderable.meshId = meshPoseData.modelMesh.meshId;
                       objectRenderable.materialId = meshPoseData.modelMesh.meshMaterialId;
                       objectRenderable.modelTransform = transformComponent.GetTransformMatrix() * meshPoseData.nodeTransform;
                       objectRenderable.shadowPass = modelRenderableComponent.shadowPass;

                       return objectRenderable;
                   });

                    std::ranges::transform(modelAnimationComponent.modelPose->boneMeshes,
                                           std::back_inserter(update.toUpdateObjectRenderables),
                                           [&](const auto& boneMesh){
                       const Render::RenderableId renderableId = renderableComponent.renderableIds[NodeMeshId::HashFunction{}(boneMesh.meshPoseData.id)];

                       Render::ObjectRenderable objectRenderable{};
                       objectRenderable.objectId = Render::ObjectId(renderableId.id);
                       objectRenderable.sceneName = renderableComponent.sceneName;
                       objectRenderable.meshId = boneMesh.meshPoseData.modelMesh.meshId;
                       objectRenderable.materialId = boneMesh.meshPoseData.modelMesh.meshMaterialId;
                       objectRenderable.modelTransform = transformComponent.GetTransformMatrix() * boneMesh.meshPoseData.nodeTransform;
                       objectRenderable.shadowPass = modelRenderableComponent.shadowPass;
                       objectRenderable.boneTransforms = boneMesh.boneTransforms;

                       return objectRenderable;
                   });
                }
                break;
                case ComponentState::Synced:
                    // no-op
                break;
            }

            renderableComponent.state = ComponentState::Synced;
        });
}

std::optional<ModelPose> RendererSyncSystem::GetModelPose(const std::string& modelName,
                                                          const std::optional<ModelAnimationState>& animationState)
{
    const auto registeredModelOpt = std::dynamic_pointer_cast<WorldResources>(m_worldResources)->GetRegisteredModel(modelName);
    if (!registeredModelOpt)
    {
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

void RendererSyncSystem::SyncTerrainRenderables(const RunState::Ptr&, entt::registry& registry, Render::WorldUpdate& update)
{
    registry.view<RenderableStateComponent, TerrainRenderableComponent, TransformComponent>().each(
        [&](const auto&,
            RenderableStateComponent& renderableComponent,
            TerrainRenderableComponent& terrainRenderableComponent,
            const TransformComponent& transformComponent)
        {
            Render::TerrainRenderable terrainRenderable{};
            terrainRenderable.terrainId = Render::TerrainId(renderableComponent.renderableIds[0].id);
            terrainRenderable.sceneName = renderableComponent.sceneName;
            terrainRenderable.materialId = terrainRenderableComponent.materialId;
            terrainRenderable.size = terrainRenderableComponent.size;
            terrainRenderable.modelTransform = transformComponent.GetTransformMatrix();
            terrainRenderable.heightMapTextureId = terrainRenderableComponent.heightMapTextureId;
            terrainRenderable.tesselationLevel = terrainRenderableComponent.tesselationLevel;
            terrainRenderable.displacementFactor = terrainRenderableComponent.displacementFactor;

            switch (renderableComponent.state)
            {
                case ComponentState::New:
                {
                    terrainRenderable.terrainId = m_renderer->GetIds()->terrainIds.GetId();
                    renderableComponent.renderableIds[0] = Render::RenderableId(terrainRenderable.terrainId.id);

                    update.toAddTerrainRenderables.push_back(terrainRenderable);
                }
                    break;
                case ComponentState::Dirty:
                {
                    update.toUpdateTerrainRenderables.push_back(terrainRenderable);
                }
                break;
                case ComponentState::Synced:
                    // no-op TODO Perf:: Don't include synced items in the loop
                break;
            }

            renderableComponent.state = ComponentState::Synced;
        });

    //
    // Deleted object renderables
    //
    for (const auto& it : m_destroyedTerrainRenderables)
    {
        for (const auto& renderableId : it.second)
        {
            update.toDeleteTerrainIds.emplace_back(renderableId.id);
        }
    }
    m_destroyedTerrainRenderables.clear();
}

void RendererSyncSystem::SyncLights(const RunState::Ptr&, entt::registry& registry, Render::WorldUpdate& update)
{
    registry.view<LightComponent, LightRenderableStateComponent, TransformComponent>().each(
        [&](const auto&,
            LightComponent& lightComponent,
            LightRenderableStateComponent& lightRenderableComponent,
            const TransformComponent& transformComponent)
        {
            auto light = Render::Light(
                lightRenderableComponent.lightId,
                lightRenderableComponent.sceneName,
                transformComponent.GetPosition(), // TODO: Light shouldn't have orientation, use transform orientation
                lightComponent.castsShadows,
                lightComponent.lightProperties
            );

            switch (lightRenderableComponent.state)
            {
                case ComponentState::New:
                {
                    light.lightId = m_renderer->GetIds()->lightIds.GetId();
                    lightRenderableComponent.lightId = light.lightId;

                    update.toAddLights.push_back(light);
                }
                break;
                case ComponentState::Dirty:
                {
                    update.toUpdateLights.push_back(light);
                }
                break;
                case ComponentState::Synced:
                    // no-op TODO Perf:: Don't include synced items in the loop
                break;
            }

            lightRenderableComponent.state = ComponentState::Synced;
        });

    //
    // Deleted lights
    //
    for (const auto& it : m_destroyedLights)
    {
        for (const auto& lightId : it.second)
        {
            update.toDeleteLightIds.emplace_back(lightId);
        }
    }
    m_destroyedLights.clear();
}

void RendererSyncSystem::OnSpriteRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId)
{
    m_destroyedSpriteRenderables[sceneName].insert(renderableId);
}

void RendererSyncSystem::OnObjectRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId)
{
    m_destroyedObjectRenderables[sceneName].insert(renderableId);
}

void RendererSyncSystem::OnTerrainRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId)
{
    m_destroyedTerrainRenderables[sceneName].insert(renderableId);
}

void RendererSyncSystem::OnLightDestroyed(const std::string& sceneName, Render::LightId lightId)
{
    m_destroyedLights[sceneName].insert(lightId);
}

}
