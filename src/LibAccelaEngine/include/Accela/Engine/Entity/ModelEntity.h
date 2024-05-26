/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_MODELENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_MODELENTITY_H

#include "Entity.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/Scene/SceneCommon.h>
#include <Accela/Engine/Component/ModelRenderableComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <optional>
#include <memory>
#include <string>

namespace Accela::Engine
{
    /**
     * Helper Entity which displays a Model in the world
     */
    class ModelEntity : public Entity
    {
        public:

            using UPtr = std::unique_ptr<ModelEntity>;

            struct Params
            {
                Params& WithModel(const ResourceIdentifier& _resource);
                Params& WithPosition(const glm::vec3& _position);
                Params& WithScale(const glm::vec3& _scale);
                Params& WithOrientation(const glm::quat& _orientation);
                Params& IncludedInShadowPass(bool _inShadowPass);

                std::optional<ResourceIdentifier> resource;
                std::optional<glm::vec3> position;
                std::optional<glm::vec3> scale;
                std::optional<glm::quat> orientation;
                std::optional<bool> inShadowPass;
            };

            [[nodiscard]] static Params Builder() { return {}; }

        private:

            struct ConstructTag{};

        public:

            [[nodiscard]] static UPtr Create(const std::shared_ptr<IEngineRuntime>& engine,
                                             const Params& params,
                                             const std::string& sceneName = DEFAULT_SCENE);

            ModelEntity(ConstructTag,
                        std::shared_ptr<IEngineRuntime> engine,
                        EntityId eid,
                        std::string sceneName,
                        Params params);
            ~ModelEntity() override;

            void Destroy() override;

            [[nodiscard]] std::optional<EntityId> GetEid() const { return m_eid; }

            void RunAnimation(const Engine::ModelAnimationState& animationState);
            void StopAnimation();

        private:

            void DestroyInternal();

            void SyncAll();

            [[nodiscard]] bool CanSyncModelComponent() const;
            void SyncModelComponent();

            [[nodiscard]] bool CanSyncTransformComponent() const;
            void SyncTransformComponent();

        private:

            std::optional<EntityId> m_eid;
            std::optional<Params> m_params;

            std::optional<ModelAnimationState> m_animationState;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_MODELENTITY_H
