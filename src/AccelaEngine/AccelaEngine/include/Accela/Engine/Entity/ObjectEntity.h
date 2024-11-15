/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_OBJECTENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_OBJECTENTITY_H

#include "Entity.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Scene/SceneCommon.h>
#include <Accela/Engine/Component/PhysicsComponent.h>

#include <Accela/Render/Id.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <optional>
#include <memory>
#include <string>

namespace Accela::Engine
{
    /**
     * Helper Entity which displays an object in the world
     */
    class ACCELA_PUBLIC ObjectEntity : public Entity
    {
        public:

            using UPtr = std::unique_ptr<ObjectEntity>;

            struct Params
            {
                Params& WithStaticMesh(const Render::MeshId& _meshId);
                Params& WithMaterial(const Render::MaterialId& _materialId);
                Params& WithPosition(const glm::vec3& _position);
                Params& WithScale(const glm::vec3& _scale);
                Params& WithOrientation(const glm::quat& _orientation);
                Params& WithPhysics(const PhysicsComponent& _physics);

                std::optional<Render::MeshId> meshId;
                std::optional<Render::MaterialId> materialId;
                std::optional<glm::vec3> position;
                std::optional<glm::vec3> scale;
                std::optional<glm::quat> orientation;
                std::optional<PhysicsComponent> physics;
            };

            [[nodiscard]] static Params Builder() { return {}; }

        private:

            struct ConstructTag{};

        public:

            [[nodiscard]] static UPtr Create(const std::shared_ptr<IEngineRuntime>& engine,
                                             const Params& params,
                                             const std::string& sceneName = DEFAULT_SCENE);

            ObjectEntity(ConstructTag,
                         std::shared_ptr<IEngineRuntime> engine,
                         EntityId eid,
                         std::string sceneName,
                         Params params);
            ~ObjectEntity() override;

            [[nodiscard]] std::optional<EntityId> GetEid() const noexcept { return m_eid; }

            void Destroy() override;

        private:

            void DestroyInternal();

            void SyncAll();

            [[nodiscard]] bool CanSyncObjectRenderableComponent() const;
            void SyncObjectRenderableComponent();

            [[nodiscard]] bool CanSyncTransformComponent() const;
            void SyncTransformComponent();

            [[nodiscard]] bool CanSyncPhysicsComponent() const;
            void SyncPhysicsComponent();

        private:

            std::optional<EntityId> m_eid;
            std::optional<Params> m_params;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_OBJECTENTITY_H
