/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldState.h"
#include "AudioSystem.h"
#include "RendererSyncSystem.h"
#include "PhysicsSyncSystem.h"
#include "ModelAnimatorSystem.h"
#include "WorldLogic.h"

#include "../Audio/AudioManager.h"
#include "../Media/MediaManager.h"
#include "../Physics/IPhysics.h"
#include "../Scene/PackageResources.h"

#include "../Component/LightRenderableStateComponent.h"
#include "../Component/PhysicsStateComponent.h"
#include "../Component/MediaComponent.h"

#include <Accela/Engine/Camera2D.h>
#include <Accela/Engine/Camera3D.h>

#include <Accela/Engine/Package/CTransformComponent.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <Accela/Engine/Component/Components.h>

#include <Accela/Common/Assert.h>

#include <algorithm>

namespace Accela::Engine
{

WorldState::WorldState(Common::ILogger::Ptr logger,
                       Common::IMetrics::Ptr metrics,
                       IWorldResources::Ptr worldResources,
                       Platform::IWindow::Ptr window,
                       Render::IRenderer::Ptr renderer,
                       AudioManagerPtr audioManager,
                       MediaManagerPtr mediaManager,
                       IPhysicsPtr physics,
                       const Render::RenderSettings& renderSettings,
                       const glm::vec2& virtualResolution)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_worldResources(std::move(worldResources))
    , m_window(std::move(window))
    , m_renderer(std::move(renderer))
    , m_audioManager(std::move(audioManager))
    , m_mediaManager(std::move(mediaManager))
    , m_physics(std::move(physics))
    , m_renderSettings(renderSettings)
    , m_virtualResolution(virtualResolution)
{
    CreateRegistryListeners();
    CreateSystems();
}

void WorldState::AssertEntityValid(EntityId entityId, std::string_view caller) const
{
    Common::Assert(
        m_registry.valid((entt::entity)entityId),
        m_logger, "{}: No such entity: {}", caller, entityId
    );
}

void WorldState::CreateRegistryListeners()
{
    m_registry.on_construct<ModelRenderableComponent>().connect<&WorldState::OnModelRenderableComponentCreated>(this);
    m_registry.on_construct<PhysicsComponent>().connect<&WorldState::OnPhysicsComponentCreated>(this);

    m_registry.on_update<SpriteRenderableComponent>().connect<&WorldState::OnSpriteRenderableComponentUpdated>(this);
    m_registry.on_update<ObjectRenderableComponent>().connect<&WorldState::OnObjectRenderableComponentUpdated>(this);
    m_registry.on_update<ModelRenderableComponent>().connect<&WorldState::OnModelRenderableComponentUpdated>(this);
    m_registry.on_update<TerrainRenderableComponent>().connect<&WorldState::OnTerrainRenderableComponentUpdated>(this);
    m_registry.on_update<LightComponent>().connect<&WorldState::OnLightComponentUpdated>(this);
    m_registry.on_update<TransformComponent>().connect<&WorldState::OnTransformComponentUpdated>(this);
    m_registry.on_update<PhysicsComponent>().connect<&WorldState::OnPhysicsComponentUpdated>(this);

    m_registry.on_destroy<SpriteRenderableComponent>().connect<&WorldState::OnSpriteRenderableComponentDestroyed>(this);
    m_registry.on_destroy<ObjectRenderableComponent>().connect<&WorldState::OnObjectRenderableComponentDestroyed>(this);
    m_registry.on_destroy<ModelRenderableComponent>().connect<&WorldState::OnModelRenderableComponentDestroyed>(this);
    m_registry.on_destroy<TerrainRenderableComponent>().connect<&WorldState::OnTerrainRenderableComponentDestroyed>(this);
    m_registry.on_destroy<LightComponent>().connect<&WorldState::OnLightComponentDestroyed>(this);
    m_registry.on_destroy<TransformComponent>().connect<&WorldState::OnTransformComponentDestroyed>(this);
    m_registry.on_destroy<PhysicsComponent>().connect<&WorldState::OnPhysicsComponentDestroyed>(this);
    m_registry.on_destroy<AudioComponent>().connect<&WorldState::OnAudioComponentDestroyed>(this);
    m_registry.on_destroy<PhysicsStateComponent>().connect<&WorldState::OnPhysicsStateComponentDestroyed>(this);
    m_registry.on_destroy<MediaComponent>().connect<&WorldState::OnMediaComponentDestroyed>(this);
}

void WorldState::CreateSystems()
{
    // Physics sync system should be run before renderer sync system so that renderer is updated to latest data
    // after physics simulation is updated
    m_physicsSyncSystem = std::make_shared<PhysicsSyncSystem>(m_logger, m_metrics, m_physics);
    m_systems.push_back(m_physicsSyncSystem);

    m_rendererSyncSystem = std::make_shared<RendererSyncSystem>(m_logger, m_metrics, m_worldResources, m_renderer);
    m_systems.push_back(m_rendererSyncSystem);

    m_audioSystem = std::make_shared<AudioSystem>(m_logger, m_audioManager);
    m_systems.push_back(m_audioSystem);

    m_systems.push_back(std::make_shared<ModelAnimatorSystem>(m_logger, m_worldResources));

    for (auto& system : m_systems)
    {
        system->Initialize(m_registry);
    }
}

void WorldState::ExecuteSystems(const RunState::Ptr& runState)
{
    for (auto& system : m_systems)
    {
        m_executingSystem = system->GetType();
        system->Execute(runState, m_registry);
    }

    m_executingSystem = std::nullopt;
}

void WorldState::SyncAudioListenerToCamera(const Camera::Ptr& camera)
{
    AudioListener audioListener{};
    audioListener.worldPosition = camera->GetPosition();
    audioListener.lookUnit = camera->GetLookUnit();
    audioListener.upUnit = camera->GetUpUnit();

    SetAudioListener(audioListener);
}

std::pair<unsigned int, unsigned int> WorldState::GetWindowDisplaySize() const
{
    return *m_window->GetWindowDisplaySize();
}

bool WorldState::SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const
{
    return m_window->SetWindowSize(size);
}

glm::vec2 WorldState::GetVirtualResolution() const noexcept
{
    return m_virtualResolution;
}

void WorldState::SetVirtualResolution(const glm::vec2& virtualResolution) noexcept
{
    m_virtualResolution = virtualResolution;
}

Render::USize WorldState::RenderSizeToVirtualSize(const Render::USize& renderSize)
{
    return Engine::RenderSizeToVirtualSize(m_renderSettings, m_virtualResolution, renderSize);
}

std::pair<glm::vec3, glm::vec3> WorldState::CameraVirtualPointToWorldRay(const std::pair<uint32_t, uint32_t>& virtualPoint,
                                                                         const Camera3D::Ptr& camera,
                                                                         const float& rayWorldLength) const
{
    const auto virtualRes = GetVirtualResolution();

    Common::Assert((float)virtualPoint.first <= virtualRes.x && (float)virtualPoint.second <= virtualRes.y,
       m_logger, "Out of range virtual point");

    //
    // Get inverse camera view and projection transforms to convert points from NDC space to world space
    //
    const glm::mat4 viewTransform = glm::lookAt(
        camera->GetPosition(),
        camera->GetPosition() + camera->GetLookUnit(),
        camera->GetUpUnit()
    );
    const auto inverseViewTransform = glm::inverse(viewTransform);

    glm::mat4 projectionTransform = glm::perspective(
        glm::radians(camera->GetFovYDegrees()),
        virtualRes.x / virtualRes.y,
        0.1f,
        rayWorldLength
    );
    projectionTransform[1][1] *= -1; // Correct for Vulkan's inverted Y-axis
    const auto inverseProjectionTransform = glm::inverse(projectionTransform);

    //
    // Calculate ray start/end in NDC space
    //
    const glm::vec4 rayStart_NDC(
        ((float)virtualPoint.first / virtualRes.x - 0.5f) * 2.0f,
        ((float)virtualPoint.second / virtualRes.y - 0.5f) * 2.0f,
        0.0f,
        1.0f
    );

    const glm::vec4 rayEnd_NDC = rayStart_NDC + glm::vec4(0, 0, 1, 0);

    //
    // Transform ray from NDC space to world space
    //
    glm::vec4 rayStart_camera = inverseProjectionTransform * rayStart_NDC;
    rayStart_camera /= rayStart_camera.w;

    glm::vec4 rayStart_world = inverseViewTransform * rayStart_camera;
    rayStart_world /= rayStart_world.w;

    glm::vec4 rayEnd_camera = inverseProjectionTransform * rayEnd_NDC;
    rayEnd_camera /= rayEnd_camera.w;

    glm::vec4 rayEnd_world = inverseViewTransform * rayEnd_camera;
    rayEnd_world /= rayEnd_world.w;

    return std::make_pair(rayStart_world, rayEnd_world);
}

std::pair<glm::vec3, glm::vec3> WorldState::CameraCenterToWorldRay(const Camera3D::Ptr& camera,
                                                                   const float& rayWorldLength) const
{
    const auto virtualRes = GetVirtualResolution();

    return CameraVirtualPointToWorldRay(
        std::make_pair((uint32_t)(virtualRes.x / 2.0f), (uint32_t)(virtualRes.y / 2.0f)),
        camera,
        rayWorldLength
    );
}

SceneState& WorldState::GetOrCreateSceneState(const std::string& sceneName)
{
    auto it = m_sceneState.find(sceneName);
    if (it == m_sceneState.end())
    {
        SceneState sceneState{};

        // Default the sprite camera to the center of the virtual area
        sceneState.spriteCamera->SetPosition(m_virtualResolution / 2.0f);

        it = m_sceneState.insert({sceneName, sceneState}).first;
    }

    return it->second;
}

EntityId WorldState::CreateEntity()
{
    const auto entityId = (EntityId)m_registry.create();
    m_logger->Log(Common::LogLevel::Debug, "WorldState::CreateEntity: Created entity id: {}", entityId);
    return entityId;
}

void WorldState::DestroyEntity(EntityId entityId)
{
    AssertEntityValid(entityId, "DestroyEntity");

    m_logger->Log(Common::LogLevel::Debug, "WorldState::DestroyEntity: Destroying entity id: {}", entityId);
    m_registry.destroy((entt::entity)entityId);
    m_highlightedEntities.erase(entityId);
}

void WorldState::DestroyAllEntities()
{
    m_logger->Log(Common::LogLevel::Debug, "WorldState::DestroyAllEntities: Destroying all entities");

    for (const auto& entity: m_registry.view<entt::entity>())
    {
        m_registry.destroy(entity);
    }

    m_highlightedEntities.clear();
}

std::vector<EntityId> WorldState::GetSpriteEntitiesAt(const glm::vec2& virtualPoint) const
{
    std::vector<std::pair<EntityId, float>> matchedSprites;

    //
    // Get all sprite entities at that render point
    //
    m_registry.view<SpriteRenderableComponent, TransformComponent>().each(
        [&](const entt::entity& eid,
            const SpriteRenderableComponent& spriteComponent,
            const TransformComponent& transformComponent
        )
        {
            if (SpriteContainsPoint(m_worldResources, m_renderSettings, m_virtualResolution, spriteComponent, transformComponent, virtualPoint))
            {
                matchedSprites.emplace_back((EntityId)eid, transformComponent.GetPosition().z);
            }
        }
    );

    //
    // Sort the entities by height, with the closest (top) first
    //
    std::ranges::sort(matchedSprites, [](const auto& p1, const auto& p2){
       return p1.second < p2.second;
    });

    //
    // Transform the sorted entities to a basic entity id vector without height data
    //
    std::vector<EntityId> sortedMatchedSprites;

    std::ranges::transform(matchedSprites, std::back_inserter(sortedMatchedSprites), [](const auto& p){
        return p.first;
    });

    return sortedMatchedSprites;
}

std::optional<EntityId> WorldState::GetTopSpriteEntityAt(const glm::vec2& virtualPoint) const
{
    const auto allEntities = GetSpriteEntitiesAt(virtualPoint);
    if (allEntities.empty())
    {
        return std::nullopt;
    }

    return allEntities.front();
}

std::optional<EntityId> WorldState::GetTopObjectEntityAt(const glm::vec2& virtualPoint) const
{
    const auto renderPoint = VirtualPointToRenderPoint(m_renderSettings, m_virtualResolution, virtualPoint);

    const auto objectId = m_renderer->GetTopObjectAtRenderPoint(renderPoint);
    if (!objectId || !objectId->IsValid())
    {
        return std::nullopt;
    }

    const auto entity = std::dynamic_pointer_cast<RendererSyncSystem>(m_rendererSyncSystem)->GetObjectEntity(*objectId);
    if (!entity)
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldState::GetTopObjectEntityAt: Found an object, but unable to determine its entity: {}", objectId->id);
        return std::nullopt;
    }

    return EntityId(*entity);
}

void WorldState::CreateConstructEntities(const Construct::Ptr& construct)
{
    const auto& entities = construct->GetEntities();

    m_logger->Log(Common::LogLevel::Info, "Creating {} entities from construct: {}", entities.size(), construct->GetName());

    for (const auto& entity : entities)
    {
        const auto eid = CreateEntity();

        for (const auto& component : entity->components)
        {
            switch (component->GetType())
            {
                case Component::Type::Transform:
                {
                    const auto cTransformComponent = std::dynamic_pointer_cast<CTransformComponent>(component);
                    AddOrUpdateComponent(eid, cTransformComponent->ToEngineComponent());
                }
                break;
                case Component::Type::ModelRenderable:
                {
                    const auto cModelRenderableComponent = std::dynamic_pointer_cast<CModelRenderableComponent>(component);
                    AddOrUpdateComponent(eid, cModelRenderableComponent->component);
                }
                break;
            }
        }
    }
}

void WorldState::HighlightEntity(EntityId entityId, bool isHighlighted)
{
    if (isHighlighted)
    {
        m_highlightedEntities.insert(entityId);
    }
    else
    {
        m_highlightedEntities.erase(entityId);
    }
}

void WorldState::ToggleHighlightEntity(EntityId entityId)
{
    if (m_highlightedEntities.contains(entityId))
    {
        m_highlightedEntities.erase(entityId);
    }
    else
    {
        m_highlightedEntities.insert(entityId);
    }
}

void WorldState::ClearEntityHighlights()
{
    m_highlightedEntities.clear();
}

void WorldState::OnModelRenderableComponentCreated(entt::registry& registry, entt::entity entity)
{
    const auto& modelRenderableComponent = registry.get<ModelRenderableComponent>(entity);

    // Attach an additional private model renderable state component to track things like the
    // current pose being rendered
    registry.emplace<ModelRenderableStateComponent>(entity, modelRenderableComponent.modelResource);
}

void WorldState::OnPhysicsComponentCreated(entt::registry& registry, entt::entity entity)
{
    PhysicsStateComponent physicsStateComponent{};
    physicsStateComponent.state = ComponentState::New;

    registry.emplace<PhysicsStateComponent>(entity, physicsStateComponent);
}

template <typename T>
void MarkStateComponentDirty(entt::registry& registry, entt::entity entity)
{
    if (!registry.any_of<T>(entity)) { return; }

    registry.patch<T>(entity, [](auto& component) { component.state = ComponentState::Dirty; });
}

void WorldState::OnSpriteRenderableComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<RenderableStateComponent>(registry, entity);
}

void WorldState::OnObjectRenderableComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<RenderableStateComponent>(registry, entity);
}

void WorldState::OnModelRenderableComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<RenderableStateComponent>(registry, entity);
}

void WorldState::OnTerrainRenderableComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<RenderableStateComponent>(registry, entity);
}

void WorldState::OnTransformComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<RenderableStateComponent>(registry, entity);
    MarkStateComponentDirty<LightRenderableStateComponent>(registry, entity);

    // If the component was updated, and not because we're syncing its data from the
    // physics system, then we want to update the physics system with the new data
    if (m_executingSystem != IWorldSystem::Type::PhysicsSync)
    {
        MarkStateComponentDirty<PhysicsStateComponent>(registry, entity);
    }
}

void WorldState::OnLightComponentUpdated(entt::registry& registry, entt::entity entity)
{
    MarkStateComponentDirty<LightRenderableStateComponent>(registry, entity);
}

void WorldState::OnPhysicsComponentUpdated(entt::registry& registry, entt::entity entity)
{
    // If the component was updated, and not because we're syncing its data from the
    // physics system, then we want to update the physics system with the new data
    if (m_executingSystem != IWorldSystem::Type::PhysicsSync)
    {
        MarkStateComponentDirty<PhysicsStateComponent>(registry, entity);
    }
}

void WorldState::OnSpriteRenderableComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<RenderableStateComponent>((EntityId)entity);
}

void WorldState::OnObjectRenderableComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<RenderableStateComponent>((EntityId)entity);
}

void WorldState::OnModelRenderableComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<RenderableStateComponent>((EntityId)entity);
    RemoveComponent<ModelRenderableStateComponent>((EntityId)entity);
}

void WorldState::OnTerrainRenderableComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<RenderableStateComponent>((EntityId)entity);
}

void WorldState::OnLightComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<LightRenderableStateComponent>((EntityId)entity);
}

void WorldState::OnTransformComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<RenderableStateComponent>((EntityId)entity);
    RemoveComponent<PhysicsStateComponent>((EntityId)entity);
}

void WorldState::OnPhysicsComponentDestroyed(entt::registry&, entt::entity entity)
{
    RemoveComponent<PhysicsStateComponent>((EntityId)entity);
}

void WorldState::OnAudioComponentDestroyed(entt::registry&, entt::entity entity)
{
    const AudioComponent& audioComponent = m_registry.get<AudioComponent>(entity);

    for (const auto& activeSound : audioComponent.activeSounds)
    {
        m_logger->Log(Common::LogLevel::Info,
          "OnAudioComponentDestroyed: Cleaning up source id: {} associated with entity id: {}", activeSound, (EntityId)entity);

        m_audioManager->DestroySource(activeSound);
    }
}

void WorldState::OnPhysicsStateComponentDestroyed(entt::registry&, entt::entity entity)
{
    auto physicsSyncSystem = std::dynamic_pointer_cast<PhysicsSyncSystem>(m_physicsSyncSystem);
    physicsSyncSystem->OnPhysicsStateComponentDestroyed((EntityId)entity);
}

void WorldState::OnMediaComponentDestroyed(entt::registry&, entt::entity entity)
{
    const MediaComponent& mediaComponent = m_registry.get<MediaComponent>(entity);

    for (const auto& mediaSession : mediaComponent.activeSessions)
    {
        m_logger->Log(Common::LogLevel::Info,
          "OnMediaComponentDestroyed: Cleaning up media session id: {} associated with entity id: {}", mediaSession.id, (EntityId)entity);

        m_mediaManager->DestroySession(mediaSession);
    }
}

void WorldState::SetWorldCamera(const std::string& sceneName, const Camera3D::Ptr& camera) noexcept
{
    GetOrCreateSceneState(sceneName).worldCamera = camera;
}

Camera3D::Ptr WorldState::GetWorldCamera(const std::string& sceneName) noexcept
{
    return GetOrCreateSceneState(sceneName).worldCamera;
}

void WorldState::SetSpriteCamera(const std::string& sceneName, const Camera2D::Ptr& camera) noexcept
{
    GetOrCreateSceneState(sceneName).spriteCamera = camera;
}

Camera2D::Ptr WorldState::GetSpriteCamera(const std::string& sceneName) noexcept
{
    return GetOrCreateSceneState(sceneName).spriteCamera;
}

void WorldState::SetAmbientLighting(const std::string& sceneName,
                                    float ambientLightIntensity,
                                    const glm::vec3& ambientLightColor)
{
    Common::Assert(ambientLightIntensity >= 0.0f && ambientLightIntensity <= 1.0f,
       m_logger, "Ambient light intensity must be in the range [0..1]");

    auto& sceneState = GetOrCreateSceneState(sceneName);
    sceneState.ambientLightIntensity = ambientLightIntensity;
    sceneState.ambientLightColor = ambientLightColor;
}

void WorldState::SetSkyBox(const std::string& sceneName,
                           const std::optional<Render::TextureId>& skyBoxTextureId,
                           const std::optional<glm::mat4>& skyBoxViewTransform)
{
    auto& sceneState = GetOrCreateSceneState(sceneName);
    sceneState.skyBoxTextureId = skyBoxTextureId;
    sceneState.skyBoxViewTransform = skyBoxViewTransform;
}

std::expected<AudioSourceId, bool> WorldState::PlayEntitySound(const EntityId& entity,
                                                               const ResourceIdentifier& resource,
                                                               const AudioSourceProperties& properties)
{
    //
    // Determine the initial local audio play location from the entity's position
    //
    glm::vec3 entityPosition{0,0,0};

    if (HasComponent<TransformComponent>(entity))
    {
        entityPosition = GetComponent<TransformComponent>(entity)->GetPosition();
    }

    //
    // Create a transient local audio source
    //
    const auto sourceId = m_audioManager->CreateLocalResourceSource(resource, properties, entityPosition, true);
    if (!sourceId)
    {
        return std::unexpected(false);
    }

    //
    // Create or update the entity's audio component to track that the source is associated with it
    //
    AudioComponent audioComponent{};

    if (HasComponent<AudioComponent>(entity))
    {
        audioComponent = GetComponent<AudioComponent>(entity).value();
    }

    audioComponent.activeSounds.insert(sourceId.value());

    AddOrUpdateComponent(entity, audioComponent);

    //
    // Play the audio source
    //
    (void)m_audioManager->PlaySource(*sourceId);

    return *sourceId;
}

std::expected<AudioSourceId, bool> WorldState::PlayGlobalSound(const ResourceIdentifier& resource, const AudioSourceProperties& properties)
{
    const auto sourceId = m_audioManager->CreateGlobalResourceSource(resource, properties, true);
    if (!sourceId)
    {
        return std::unexpected(false);
    }

    (void)m_audioManager->PlaySource(*sourceId);

    return sourceId;
}

void WorldState::StopGlobalSound(AudioSourceId sourceId)
{
    m_audioManager->DestroySource(sourceId);
}

void WorldState::SetAudioListener(const AudioListener& listener)
{
    std::dynamic_pointer_cast<AudioSystem>(m_audioSystem)->SetAudioListener(listener);
}

std::expected<MediaSessionId, bool> WorldState::StartMediaSession(const PackageResourceIdentifier& resource,
                                                                  const AudioSourceProperties& audioSourceProperties,
                                                                  bool associatedWithEntity)
{
    const auto package = std::dynamic_pointer_cast<PackageResources>(m_worldResources->Packages())
        ->GetPackageSource(*resource.GetPackageName());
    if (!package)
    {
        LogError("WorldState::StartMediaSession: No such package is loaded: {}", resource.GetPackageName()->name);
        return std::unexpected(false);
    }

    const auto videoUrl = (*package)->GetVideoUrl(resource.GetResourceName());
    if (!videoUrl)
    {
        LogError("WorldState::StartMediaSession: Failed to get video url from package: {}", resource.GetResourceName());
        return std::unexpected(false);
    }

    return StartMediaSession(*videoUrl, audioSourceProperties, associatedWithEntity);
}

std::expected<MediaSessionId, bool> WorldState::StartMediaSession(const std::string& url,
                                                                  const AudioSourceProperties& audioSourceProperties,
                                                                  bool associatedWithEntity)
{
    const auto sessionId = m_mediaManager->CreateURLMediaSession(url, audioSourceProperties, associatedWithEntity);
    if (!sessionId)
    {
        LogError("WorldState::StartMediaSession: Failed to load url video: {}", url);
        return std::unexpected(false);
    }

    return *sessionId;
}

std::optional<Render::TextureId> WorldState::GetMediaSessionTextureId(const MediaSessionId& mediaSessionId) const
{
    return m_mediaManager->GetMediaSessionTextureId(mediaSessionId);
}

bool WorldState::AssociateMediaSessionWithEntity(const MediaSessionId& mediaSessionId, const EntityId& entityId)
{
    AssertEntityValid(entityId, "AssociateMediaSessionWithEntity");

    const auto audioSourceId = m_mediaManager->GetMediaSessionAudioSourceId(mediaSessionId);
    if (!audioSourceId)
    {
        LogError("WorldState::AssociateMediaSessionWithEntity: Media session doesn't exist or has no audio source");
        return false;
    }

    //
    // Create or update the entity's internal MediaComponent component. Keeps track of the fact that the
    // entity has a media session associated with it.
    //
    MediaComponent mediaComponent{};

    if (HasComponent<MediaComponent>(entityId))
    {
        mediaComponent = GetComponent<MediaComponent>(entityId).value();
    }

    mediaComponent.activeSessions.insert(mediaSessionId);

    AddOrUpdateComponent(entityId, mediaComponent);

    //
    // Create or update the entity's internal AudioComponent component.
    //
    AudioComponent audioComponent{};

    if (HasComponent<AudioComponent>(entityId))
    {
        audioComponent = GetComponent<AudioComponent>(entityId).value();
    }

    audioComponent.activeSounds.insert(audioSourceId.value());

    AddOrUpdateComponent(entityId, audioComponent);

    return true;
}

std::future<bool> WorldState::MediaSessionPlay(const MediaSessionId& mediaSessionId, const std::optional<MediaPoint>& playPoint) const
{
    return m_mediaManager->PlayMediaSession(mediaSessionId, playPoint);
}

std::future<bool> WorldState::MediaSessionPause(const MediaSessionId& mediaSessionId) const
{
    return m_mediaManager->PauseMediaSession(mediaSessionId);
}

std::future<bool> WorldState::MediaSessionStop(const MediaSessionId& mediaSessionId) const
{
    return m_mediaManager->StopMediaSession(mediaSessionId);
}

std::future<bool> WorldState::MediaSessionSeekByOffset(const MediaSessionId& mediaSessionId, const MediaDuration& offset) const
{
    return m_mediaManager->SeekMediaSessionByOffset(mediaSessionId, offset);
}

std::future<bool> WorldState::MediaSessionLoadStreams(const MediaSessionId& mediaSessionId, const std::unordered_set<unsigned int>& streamIndices) const
{
    return m_mediaManager->LoadStreams(mediaSessionId, streamIndices);
}

IPhysicsRuntime::Ptr WorldState::GetPhysics() const
{
    return std::dynamic_pointer_cast<IPhysicsRuntime>(m_physics);
}

void WorldState::MarkSpritesDirty()
{
    m_registry.view<RenderableStateComponent, SpriteRenderableComponent>().each(
    [&](const auto&,
            RenderableStateComponent& renderableComponent,
            SpriteRenderableComponent&)
        {
            renderableComponent.state = ComponentState::Dirty;
        }
    );
}

Render::RenderSettings WorldState::GetRenderSettings() const noexcept
{
    return m_renderSettings;
}

void WorldState::SetRenderSettings(const Render::RenderSettings& renderSettings) noexcept
{
    m_renderSettings = renderSettings;
}

std::unordered_set<EntityId> WorldState::GetHighlightedEntities() const noexcept
{
    return m_highlightedEntities;
}

}
