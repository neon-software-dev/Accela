/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SPRITEENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SPRITEENTITY_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Entity/Entity.h>
#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <optional>

namespace Accela::Engine
{
    /**
     * Helper Entity which displays a Sprite on the screen (in 2D screen space)
     */
    class SpriteEntity : public Entity
    {
        public:

            using Ptr = std::shared_ptr<SpriteEntity>;
            using UPtr = std::unique_ptr<SpriteEntity>;

            struct Params
            {
                Params& WithTextureId(Render::TextureId _textureId);
                Params& WithSourcePixelRect(const Render::URect& _srcPixelRect);
                Params& WithVirtualSize(const Render::FSize& _dstVirtualSize);
                Params& WithPosition(const glm::vec3& _position);
                Params& WithScale(const glm::vec2& _scale);
                Params& WithOrientation(const glm::quat& orientation);

                std::optional<Render::TextureId> textureId;
                std::optional<Render::URect> srcPixelRect;
                std::optional<Render::FSize> dstVirtualSize;

                std::optional<glm::vec3> position;
                std::optional<glm::vec2> scale;
                std::optional<glm::quat> orientation;
            };

            [[nodiscard]] static Params Builder() { return {}; }

        private:

            struct ConstructTag{};

        public:

            static SpriteEntity::UPtr Create(const std::shared_ptr<IEngineRuntime>& engine,
                                             const Params& params = {},
                                             const std::string& sceneName = DEFAULT_SCENE);

            SpriteEntity(ConstructTag,
                         std::shared_ptr<IEngineRuntime> engine,
                         EntityId eid,
                         std::string sceneName,
                         Params params);
            ~SpriteEntity() override;

            SpriteEntity(const SpriteEntity&) = delete;
            SpriteEntity& operator=(const SpriteEntity&) = delete;

            [[nodiscard]] std::optional<EntityId> GetEid() const;

            [[nodiscard]] std::optional<Render::TextureId> GetTextureId() const noexcept;
            void SetTextureById(Render::TextureId textureId);
            [[nodiscard]] bool SetTextureByAssetName(const std::string& assetName);

            [[nodiscard]] std::optional<Render::URect> GetSourcePixelRect() const noexcept;
            void SetSourcePixelRect(const Render::URect& srcPixelRect);

            [[nodiscard]] std::optional<Render::FSize> GetDstVirtualSize() const noexcept;
            void SetDstVirtualSize(const Render::FSize& dstVirtualSize);

            [[nodiscard]] std::optional<glm::vec3> GetPosition() const noexcept;
            void SetPosition(const glm::vec3& position);

            [[nodiscard]] std::optional<glm::vec2> GetScale() const noexcept;
            void SetScale(const glm::vec2& scale);

            [[nodiscard]] std::optional<glm::quat> GetOrientation() const noexcept;
            void SetOrientation(const glm::quat& orientation);

            void Destroy() override;

        private:

            void DestroyInternal();

            void SyncAll();

            [[nodiscard]] bool CanSyncSpriteComponent() const;
            void SyncSpriteComponent();

            [[nodiscard]] bool CanSyncTransformComponent() const;
            void SyncTransformComponent();

        private:

            std::optional<EntityId> m_eid;
            std::optional<Params> m_params;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SPRITEENTITY_H
