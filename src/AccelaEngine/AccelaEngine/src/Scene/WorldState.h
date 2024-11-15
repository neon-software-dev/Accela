/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_WORLDSTATE_H
#define LIBACCELAENGINE_SRC_SCENE_WORLDSTATE_H

#include "IWorldSystem.h"
#include "SceneState.h"

#include "../ForwardDeclares.h"
#include "../RunState.h"

#include <Accela/Engine/Camera.h>
#include <Accela/Engine/Scene/IWorldState.h>
#include <Accela/Engine/Scene/IWorldResources.h>
#include <Accela/Engine/Audio/AudioCommon.h>

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Light.h>

#include <Accela/Platform/Window/IWindow.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <entt/entt.hpp>

#include <expected>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Accela::Engine
{
    class WorldState : public IWorldState
    {
        public:

            WorldState(Common::ILogger::Ptr logger,
                       Common::IMetrics::Ptr metrics,
                       IWorldResources::Ptr worldResources,
                       Platform::IWindow::Ptr window,
                       Render::IRenderer::Ptr renderer,
                       AudioManagerPtr audioManager,
                       MediaManagerPtr mediaManager,
                       IPhysicsPtr physics,
                       const Render::RenderSettings& renderSettings,
                       const glm::vec2& virtualResolution);

            void ExecuteSystems(const RunState::Ptr& runState);
            void SyncAudioListenerToCamera(const Camera::Ptr& camera);
            [[nodiscard]] SceneState& GetOrCreateSceneState(const std::string& sceneName);
            void MarkSpritesDirty();
            [[nodiscard]] Render::RenderSettings GetRenderSettings() const noexcept;
            void SetRenderSettings(const Render::RenderSettings& renderSettings) noexcept;
            [[nodiscard]] std::unordered_set<EntityId> GetHighlightedEntities() const noexcept;

            //
            // Entity
            //
            [[nodiscard]] EntityId CreateEntity() override;
            void DestroyEntity(EntityId entityId) override;
            void DestroyAllEntities() override;

            [[nodiscard]] std::vector<EntityId> GetSpriteEntitiesAt(const glm::vec2& virtualPoint) const override;
            [[nodiscard]] std::optional<EntityId> GetTopSpriteEntityAt(const glm::vec2& virtualPoint) const override;
            [[nodiscard]] std::optional<EntityId> GetTopObjectEntityAt(const glm::vec2& virtualPoint) const override;

            void CreateConstructEntities(const Construct::Ptr& construct) override;

            void HighlightEntity(EntityId entityId, bool isHighlighted) override;
            void ToggleHighlightEntity(EntityId entityId) override;
            void ClearEntityHighlights() override;

            template <typename T>
            bool HasComponent(EntityId entityId)
            {
                AssertEntityValid(entityId, "HasComponent");

                return m_registry.any_of<T>((entt::entity)entityId);
            }

            template <typename T>
            void AddOrUpdateComponent(EntityId entityId, const T& component)
            {
                AssertEntityValid(entityId, "AddOrUpdateComponent");

                m_registry.emplace_or_replace<T>((entt::entity)entityId, component);
            }

            template <typename T>
            void RemoveComponent(EntityId entityId)
            {
                AssertEntityValid(entityId, "RemoveComponent");

                m_registry.remove<T>((entt::entity)entityId);
            }

            template <typename T>
            std::optional<T> GetComponent(EntityId entityId)
            {
                AssertEntityValid(entityId, "GetComponent");

                if (m_registry.any_of<T>((entt::entity)entityId))
                {
                    return m_registry.get<T>((entt::entity)entityId);
                }

                return std::nullopt;
            }

            //
            // Windowing
            //
            [[nodiscard]] std::pair<unsigned int, unsigned int> GetWindowDisplaySize() const override;
            [[nodiscard]] bool SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const override;

            //
            // Virtual Resolution
            //
            [[nodiscard]] glm::vec2 GetVirtualResolution() const noexcept override;
            void SetVirtualResolution(const glm::vec2& virtualResolution) noexcept override;
            [[nodiscard]] Render::USize RenderSizeToVirtualSize(const Render::USize& renderSize) override;
            [[nodiscard]] std::pair<glm::vec3, glm::vec3> CameraVirtualPointToWorldRay(
                const std::pair<uint32_t, uint32_t>& virtualPoint,
                const Camera3D::Ptr& camera,
                const float& rayWorldLength) const override;
            [[nodiscard]] std::pair<glm::vec3, glm::vec3> CameraCenterToWorldRay(
                const Camera3D::Ptr& camera,
                const float& rayWorldLength) const override;

            //
            // Camera
            //
            void SetWorldCamera(const std::string& sceneName, const Camera3D::Ptr& camera) noexcept override;
            [[nodiscard]] Camera3D::Ptr GetWorldCamera(const std::string& sceneName) noexcept override;
            void SetSpriteCamera(const std::string& sceneName, const Camera2D::Ptr& camera) noexcept override;
            [[nodiscard]] Camera2D::Ptr GetSpriteCamera(const std::string& sceneName) noexcept override;

            //
            // Lighting
            //
            void SetAmbientLighting(const std::string& sceneName,
                                    float ambientLightIntensity,
                                    const glm::vec3& ambientLightColor) override;

            //
            // SkyMap
            //
            void SetSkyBox(const std::string& sceneName,
                           const std::optional<Render::TextureId>& skyBoxTextureId,
                           const std::optional<glm::mat4>& skyBoxViewTransform) override;

            //
            // Audio
            //
            [[nodiscard]] std::expected<AudioSourceId, bool> PlayEntitySound(
                const EntityId& entity,
                const ResourceIdentifier& resource,
                const AudioSourceProperties& properties) override;

            [[nodiscard]] std::expected<AudioSourceId, bool> PlayGlobalSound(
                const ResourceIdentifier& resource,
                const AudioSourceProperties& properties) override;

            void StopGlobalSound(AudioSourceId sourceId) override;

            void SetAudioListener(const AudioListener& listener) override;

            //
            // Media
            //
            [[nodiscard]] std::expected<MediaSessionId, bool> StartMediaSession(const PackageResourceIdentifier& resource,
                                                                                const AudioSourceProperties& audioSourceProperties,
                                                                                bool associatedWithEntity) override;
            [[nodiscard]] std::expected<MediaSessionId, bool> StartMediaSession(const std::string& url,
                                                                                const AudioSourceProperties& audioSourceProperties,
                                                                                bool associatedWithEntity) override;
            [[nodiscard]] std::optional<Render::TextureId> GetMediaSessionTextureId(const MediaSessionId& mediaSessionId) const override;
            [[nodiscard]] bool AssociateMediaSessionWithEntity(const MediaSessionId& mediaSessionId, const EntityId& entityId) override;
            [[nodiscard]] std::future<bool> MediaSessionPlay(const MediaSessionId& mediaSessionId, const std::optional<MediaPoint>& playPoint) const override;
            [[nodiscard]] std::future<bool> MediaSessionPause(const MediaSessionId& mediaSessionId) const override;
            [[nodiscard]] std::future<bool> MediaSessionStop(const MediaSessionId& mediaSessionId) const override;
            [[nodiscard]] std::future<bool> MediaSessionSeekByOffset(const MediaSessionId& mediaSessionId, const MediaDuration& offset) const override;
            [[nodiscard]] std::future<bool> MediaSessionLoadStreams(const MediaSessionId& mediaSessionId, const std::unordered_set<unsigned int>& streamIndices) const override;

            //
            // Physics
            //
            [[nodiscard]] IPhysicsRuntime::Ptr GetPhysics() const override;

        private:

            void AssertEntityValid(EntityId entityId, std::string_view caller) const;

            void CreateRegistryListeners();
            void CreateSystems();

            void OnModelRenderableComponentCreated(entt::registry& registry, entt::entity entity);
            void OnPhysicsComponentCreated(entt::registry& registry, entt::entity entity);

            void OnSpriteRenderableComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnObjectRenderableComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnModelRenderableComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnTerrainRenderableComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnLightComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnTransformComponentUpdated(entt::registry& registry, entt::entity entity);
            void OnPhysicsComponentUpdated(entt::registry& registry, entt::entity entity);

            void OnSpriteRenderableComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnObjectRenderableComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnModelRenderableComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnTerrainRenderableComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnLightComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnTransformComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnPhysicsComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnAudioComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnPhysicsStateComponentDestroyed(entt::registry& registry, entt::entity entity);
            void OnMediaComponentDestroyed(entt::registry& registry, entt::entity entity);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            IWorldResources::Ptr m_worldResources;
            Platform::IWindow::Ptr m_window;
            Render::IRenderer::Ptr m_renderer;
            AudioManagerPtr m_audioManager;
            MediaManagerPtr m_mediaManager;
            // TODO: entt::registry<uint64_t>? (https://github.com/skypjack/entt/issues/197)
            entt::registry m_registry;
            IPhysicsPtr m_physics;

            std::vector<IWorldSystem::Ptr> m_systems;
            IWorldSystem::Ptr m_rendererSyncSystem;
            IWorldSystem::Ptr m_audioSystem;
            IWorldSystem::Ptr m_physicsSyncSystem;
            std::optional<IWorldSystem::Type> m_executingSystem;

            Render::RenderSettings m_renderSettings;
            glm::vec2 m_virtualResolution;
            std::unordered_map<std::string, SceneState> m_sceneState;
            std::unordered_set<EntityId> m_highlightedEntities;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_WORLDSTATE_H
