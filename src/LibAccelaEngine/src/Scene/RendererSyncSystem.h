/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H

#include "IWorldSystem.h"

#include "../Component/RenderableStateComponent.h"
#include "../Component/ModelRenderableStateComponent.h"

#include "../Model/ModelPose.h"

#include <Accela/Engine/Scene/IWorldResources.h>
#include <Accela/Engine/Component/ModelRenderableComponent.h>
#include <Accela/Engine/Component/SpriteRenderableComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Light.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <entt/entt.hpp>

#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace Accela::Engine
{
    class RendererSyncSystem : public IWorldSystem
    {
        public:

            RendererSyncSystem(Common::ILogger::Ptr logger,
                               Common::IMetrics::Ptr metrics,
                               IWorldResources::Ptr worldResources,
                               Render::IRenderer::Ptr renderer);

            [[nodiscard]] Type GetType() const noexcept override { return Type::RendererSync; };

            void Initialize(entt::registry& registry) override;
            void Execute(const RunState::Ptr& runState, entt::registry& registry) override;

        private:

            void OnRenderableStateDestroyed(entt::registry& registry, entt::entity entity);
            void OnLightRenderableStateDestroyed(entt::registry& registry, entt::entity entity);

            void ProcessNewlyCompletedRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void ProcessUpdatedRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void ProcessRenderablesToDestroy(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);

            void CompleteSpriteRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update, const glm::vec3& virtualToRenderRatio);
            void CompleteObjectRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update);
            void CompleteModelRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update);
            void CompleteLightRenderable(entt::registry& registry, entt::entity entity, Render::WorldUpdate& update);

            [[nodiscard]] static Render::SpriteRenderable GetSpriteRenderable(entt::registry& registry, entt::entity entity, const glm::vec3& virtualToRenderRatio);
            [[nodiscard]] static Render::ObjectRenderable GetObjectRenderable(entt::registry& registry, entt::entity entity);

            [[nodiscard]] static std::vector<std::pair<std::size_t, Render::ObjectRenderable>> GetModelRenderables(
                RenderableStateComponent& stateComponent,
                const ModelRenderableComponent& modelComponent,
                const ModelRenderableStateComponent& modelStateComponent,
                const TransformComponent& transformComponent);
            [[nodiscard]] static Render::ObjectRenderable GetModelRenderable(RenderableStateComponent& stateComponent,
                                                                             const ModelRenderableComponent& modelComponent,
                                                                             const TransformComponent& transformComponent,
                                                                             const MeshPoseData& meshPoseData);
            [[nodiscard]] static Render::ObjectRenderable GetModelRenderable(RenderableStateComponent& stateComponent,
                                                                             const ModelRenderableComponent& modelComponent,
                                                                             const TransformComponent& transformComponent,
                                                                             const BoneMesh& boneMesh);

            [[nodiscard]] static Render::Light GetLightRenderable(entt::registry& registry, entt::entity entity);

            [[nodiscard]] std::optional<ModelPose> GetModelPose(const ResourceIdentifier& model, const std::optional<ModelAnimationState>& animationState);

            [[nodiscard]] static glm::vec3 GetVirtualToRenderRatio(const RunState::Ptr& runState);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResources::Ptr m_worldResources;
            Render::IRenderer::Ptr m_renderer;

            entt::observer m_spriteCompletedObserver;
            entt::observer m_objectCompletedObserver;
            entt::observer m_modelCompletedObserver;
            entt::observer m_lightCompletedObserver;

            entt::observer m_renderableStateUpdateObserver;
            entt::observer m_lightStateUpdateObserver;

            std::unordered_set<Render::RenderableId> m_spriteRenderablesToDestroy;
            std::unordered_set<Render::RenderableId> m_objectRenderablesToDestroy;
            std::unordered_set<Render::LightId> m_lightsToDestroy;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H
