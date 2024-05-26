#ifndef LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H
#define LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H

#include "IWorldSystem.h"

#include "../Model/ModelPose.h"

#include <Accela/Engine/Scene/IWorldResources.h>
#include <Accela/Engine/Component/ModelRenderableComponent.h>

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

            void Execute(const RunState::Ptr& runState, entt::registry& registry) override;

            void OnSpriteRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId);
            void OnObjectRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId);
            void OnTerrainRenderableDestroyed(const std::string& sceneName, Render::RenderableId renderableId);
            void OnLightDestroyed(const std::string& sceneName, Render::LightId lightId);

        private:

            void SyncSpriteRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void SyncObjectRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void SyncModelRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void SyncTerrainRenderables(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);
            void SyncLights(const RunState::Ptr& runState, entt::registry& registry, Render::WorldUpdate& update);

            [[nodiscard]] std::optional<ModelPose> GetModelPose(const ResourceIdentifier& model,
                                                                const std::optional<ModelAnimationState>& animationState);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResources::Ptr m_worldResources;
            Render::IRenderer::Ptr m_renderer;

            std::unordered_map<std::string, std::unordered_set<Render::RenderableId>> m_destroyedSpriteRenderables;
            std::unordered_map<std::string, std::unordered_set<Render::RenderableId>> m_destroyedObjectRenderables;
            std::unordered_map<std::string, std::unordered_set<Render::RenderableId>> m_destroyedTerrainRenderables;
            std::unordered_map<std::string, std::unordered_set<Render::LightId>> m_destroyedLights;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_RENDERERSYNCSYSTEM_H
