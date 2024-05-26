/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDSTATE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDSTATE_H

#include "../Common.h"
#include "../ResourceIdentifier.h"
#include "../Camera2D.h"
#include "../Camera3D.h"

#include "../Audio/AudioCommon.h"
#include "../Audio/AudioSourceProperties.h"
#include "../Audio/AudioListener.h"

#include "../Physics/IPhysicsRuntime.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>

#include <glm/glm.hpp>

#include <expected>
#include <memory>
#include <vector>
#include <optional>
#include <utility>

namespace Accela::Engine
{
    /**
     * Main user-facing interface to manipulating the state of the game world and of the engine itself
     *
     * TODO: Hide away most methods into subsystems once the class grows too large
     */
    class IWorldState
    {
        public:

            using Ptr = std::shared_ptr<IWorldState>;

        public:

            virtual ~IWorldState() = default;

            //
            // Entities
            //

            /**
             * Create a new entity which components can be attached to
             *
             * @return The EntityId of the created entity
             */
            [[nodiscard]] virtual EntityId CreateEntity() = 0;

            /**
             * Destroy a previously create entity (and all components attached to it)
             *
             * @param entityId The EntityId of the entity to be destroyed
             */
            virtual void DestroyEntity(EntityId entityId) = 0;

            /**
             * Destroys all entities, across all scenes
             */
            virtual void DestroyAllEntities() = 0;

            /**
             * Returns all sprite entities which exist underneath the provided virtual point
             *
             * @param virtualPoint The virtual point in question
             *
             * @return The list of EntityIds associated with sprites at the given virtual point, sorted from
             * top to bottom
             */
            [[nodiscard]] virtual std::vector<EntityId> GetSpriteEntitiesAt(const glm::vec2& virtualPoint) const = 0;

            /**
             * Return the top-most sprite, if any, underneath the provided virtual point
             *
             * @param virtualPoint The virtual point in question
             *
             * @return The EntityId of the top-most sprite underneath the virtual point, or std::nullopt if no such sprite
             */
            [[nodiscard]] virtual std::optional<EntityId> GetTopSpriteEntityAt(const glm::vec2& virtualPoint) const = 0;

            //
            // Windowing
            //

            /**
             * The resolution for the display the engine's window is running on
             */
            [[nodiscard]] virtual std::pair<unsigned int, unsigned int> GetWindowDisplaySize() const = 0;


            /**
             * Sets the engine's window size to a new size
             *
             * @param size The new size for the window
             *
             * @return Whether or not the operation was successful
             */
            [[nodiscard]] virtual bool SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const = 0;

            //
            // Virtual Resolution
            //

            /**
             * @return The virtual resolution the engine is currently configured for
             */
            [[nodiscard]] virtual glm::vec2 GetVirtualResolution() const noexcept = 0;

            /**
             * Sets the engine's virtual resolution
             *
             * @param virtualResolution The new virtual resolution to be used
             */
            virtual void SetVirtualResolution(const glm::vec2& virtualResolution) noexcept = 0;

            /**
             * Converts a render/pixel size to a size in equivalent virtual space.
             *
             * For example, if the virtual resolution is half the render resolution, then passing in
             * 100x100 would return 50x50.
             *
             * @param renderSize The render/pixel size in question
             *
             * @return The equivalent virtual space size
             */
            [[nodiscard]] virtual Render::USize RenderSizeToVirtualSize(const Render::USize& renderSize) = 0;

            /**
             * Converts a point in virtual screen space, given a specific camera, and returns a ray that
             * emanates from that point in world space.
             *
             * @param virtualPoint The virtual space point
             * @param camera The camera to use
             * @param rayWorldLength The final world length of the ray
             *
             * @return A start/end ray in world space
             */
            [[nodiscard]] virtual std::pair<glm::vec3, glm::vec3> CameraVirtualPointToWorldRay(
                const std::pair<uint32_t, uint32_t>& virtualPoint,
                const Camera3D::Ptr& camera,
                const float& rayWorldLength) const = 0;

            /**
             * Same as CameraVirtualPointToWorldRay, except a special case helper function which uses the center
             * of the screen/virtual space as the ray emanation point, rather than an arbitrary virtual point.
             */
            [[nodiscard]] virtual std::pair<glm::vec3, glm::vec3> CameraCenterToWorldRay(
                const Camera3D::Ptr& camera,
                const float& rayWorldLength) const = 0;

            //
            // Camera
            //

            /**
             * Manually set the world camera for a specific scene
             *
             * @param sceneName The scene to be affected
             * @param camera The world camera to be used
             */
            virtual void SetWorldCamera(const std::string& sceneName, const Camera3D::Ptr& camera) noexcept = 0;

            /**
             * @return The current world camera for the specified scene
             */
            [[nodiscard]] virtual Camera3D::Ptr GetWorldCamera(const std::string& sceneName) noexcept = 0;

            /**
             * Manually set the sprite camera for a specific scene
             * @param sceneName The scene to be affected
             * @param camera The sprite camera to be used
             */
            virtual void SetSpriteCamera(const std::string& sceneName, const Camera2D::Ptr& camera) noexcept = 0;

            /**
             * @return The current sprite camera for the specified scene
             */
            [[nodiscard]] virtual Camera2D::Ptr GetSpriteCamera(const std::string& sceneName) noexcept = 0;

            //
            // Lighting
            //

            /**
             * Configure the ambient world lighting settings
             *
             * @param sceneName The scene to configure the settings for
             * @param ambientLightIntensity The intensity of the ambient light [0..1]
             * @param ambientLightColor The color of the ambient light
             */
            virtual void SetAmbientLighting(const std::string& sceneName,
                                            float ambientLightIntensity,
                                            const glm::vec3& ambientLightColor) = 0;

            //
            // SkyMap
            //

            /**
             * Enable a world-space skybox
             * @param sceneName The scene to be affected
             * @param skyBoxTextureId The TextureId of the cubic sky-map texture to be displayed, or std::nullopt if
             * the active skybox should be removed
             * @param skyBoxViewTransform An optional additional view transform to apply to the skybox
             */
            virtual void SetSkyBox(const std::string& sceneName,
                                   const std::optional<Render::TextureId>& skyBoxTextureId,
                                   const std::optional<glm::mat4>& skyBoxViewTransform = std::nullopt) = 0;

            //
            // Audio
            //

            /**
             * Play a sound associated in world space with a particular Entity
             *
             * @param entity The entity the sound should be emitted from
             * @param resource Identifies the sound resource to be played
             * @param properties Properties for how the sound should be displayed
             *
             * @return An AudioSourceId associated with the sound play instance, or false on error
             */
            virtual std::expected<AudioSourceId, bool> PlayEntitySound(const EntityId& entity,
                                                                       const ResourceIdentifier& resource,
                                                                       const AudioSourceProperties& properties) = 0;

            /**
             * Play a package sound globally, not attached to any particular entity
             *
             * @param resource Identifies the sound resource to be played
             * @param properties Properties for how the sound should be displayed
             *
             * @return An AudioSourceId associated with the sound play instance, or false on error
             */
            virtual std::expected<AudioSourceId, bool> PlayGlobalSound(
                const ResourceIdentifier& resource,
                const AudioSourceProperties& properties) = 0;

            /**
             * Stops a global sound that was previously started via a call to PlayGlobalSound(..)
             *
             * @param sourceId The AudioSourceId of the global sound to be stopped
             */
            virtual void StopGlobalSound(AudioSourceId sourceId) = 0;

            /**
             * Sets the properties of the audio listener. Usually tightly tied to the position of the
             * "player" within the world.
             *
             * @param listener The properties of the audio listener
             */
            virtual void SetAudioListener(const AudioListener& listener) = 0;

            //
            // Physics
            //

            /**
             * @return A user-facing interface to the physics system
             */
            [[nodiscard]] virtual IPhysicsRuntime::Ptr GetPhysics() const = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IWORLDSTATE_H
