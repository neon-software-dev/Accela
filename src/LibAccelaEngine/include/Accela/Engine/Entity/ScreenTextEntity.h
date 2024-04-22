/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCREENTEXTENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCREENTEXTENTITY_H

#include "Entity.h"

#include "../IEngineRuntime.h"
#include "../Common.h"

#include "../Scene/SceneCommon.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Platform/Text/TextProperties.h>

#include <glm/glm.hpp>

#include <string>
#include <cstdint>
#include <optional>
#include <memory>

namespace Accela::Engine
{
    enum class TextLayoutMode
    {
        Center,
        TopLeft
    };

    /**
     * Helper Entity which displays text on the screen (in 2D screen space)
     */
    class ScreenTextEntity : public Entity
    {
        public:

            using Ptr = std::shared_ptr<ScreenTextEntity>;
            using UPtr = std::unique_ptr<ScreenTextEntity>;

            struct Params
            {
                Params& WithText(const std::string& _text);
                Params& WithProperties(const Platform::TextProperties& _properties);
                Params& WithTextLayoutMode(TextLayoutMode _textLayoutMode);
                Params& WithPosition(const glm::vec3& _position);

                std::optional<std::string> text;
                std::optional<Platform::TextProperties> properties;
                std::optional<TextLayoutMode> textLayoutMode;
                std::optional<glm::vec3> position;
            };

            [[nodiscard]] static Params Builder() { return {}; }

        private:

            struct ConstructTag{};

        public:

            static ScreenTextEntity::UPtr Create(const IEngineRuntime::Ptr& engine,
                                                 const Params& params = {},
                                                 const std::string& sceneName = DEFAULT_SCENE);

            ScreenTextEntity(ConstructTag, IEngineRuntime::Ptr engine, EntityId eid, std::string sceneName, const Params& params);
            ~ScreenTextEntity() override;

            ScreenTextEntity(const ScreenTextEntity&) = delete;
            ScreenTextEntity& operator=(const ScreenTextEntity&) = delete;

            void Destroy() override;

            [[nodiscard]] std::optional<EntityId> GetEid() const;

            [[nodiscard]] std::optional<std::string> GetText() const;
            bool SetText(const std::string& text);

            [[nodiscard]] std::optional<Platform::TextProperties> GetTextProperties() const;
            bool SetTextProperties(const Platform::TextProperties& properties);

            [[nodiscard]] TextLayoutMode GetTextLayoutMode() const;
            bool SetTextLayoutMode(TextLayoutMode textLayoutMode);

            [[nodiscard]] std::optional<glm::vec3> GetPosition() const;
            bool SetPosition(const glm::vec3& position);

            [[nodiscard]] std::optional<Render::USize> GetRenderedTextSize() const;

        private:

            void DestroyInternal();

            void SyncAll();

            bool CanSyncText();
            bool SyncText();

            bool CanSyncPosition();
            bool SyncPosition();

        private:

            std::optional<EntityId> m_eid;

            std::optional<std::string> m_text;
            std::optional<Platform::TextProperties> m_properties;
            TextLayoutMode m_textLayoutMode{TextLayoutMode::TopLeft};
            std::optional<glm::vec3> m_position;

            std::optional<TextRender> m_textRender;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCREENTEXTENTITY_H
