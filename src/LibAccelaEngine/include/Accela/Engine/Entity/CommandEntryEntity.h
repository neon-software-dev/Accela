/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_COMMANDENTRYENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_COMMANDENTRYENTITY_H

#include "Entity.h"

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Scene/SceneCommon.h>
#include <Accela/Engine/Scene/TextRender.h>

#include <Accela/Platform/Text/TextProperties.h>

#include <optional>

namespace Accela::Engine
{
    /**
     * Helper Entity which provides a single line command-prompt type text view
     */
    class CommandEntryEntity : public Entity
    {
        public:

            using Ptr = std::shared_ptr<CommandEntryEntity>;
            using UPtr = std::unique_ptr<CommandEntryEntity>;

        private:

            struct ConstructTag{};

        public:

            static CommandEntryEntity::UPtr Create(const std::shared_ptr<IEngineRuntime>& engine,
                                                   const Platform::TextProperties& textProperties,
                                                   bool ignoreFirstAppend,
                                                   const std::string& sceneName = DEFAULT_SCENE);

            CommandEntryEntity(ConstructTag,
                               std::shared_ptr<IEngineRuntime> engine,
                               Platform::TextProperties textProperties,
                               std::string sceneName,
                               EntityId eid,
                               bool ignoreFirstAppend);
            ~CommandEntryEntity() override;

            void Destroy() override;

            [[nodiscard]] std::string GetEntry() const noexcept;

            void SetEntry(const std::string& entry);
            void AppendToEntry(const std::string& text);
            void DeleteLastEntryChar();
            void ClearEntry();

        private:

            void DestroyInternal();

            bool SyncText();

        private:

            Platform::TextProperties m_textProperties;
            std::optional<EntityId> m_eid;

            // Kind of a hack to just account for pressing a button to open the entry spawning both
            // a key event and a text input event; we don't want to type that initial key into
            // the now opened command entry if it's processed second, we want to just ignore it.
            bool m_ignoreNextAppend{false};

            std::optional<TextRender> m_textRender;
            std::string m_entry;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_COMMANDENTRYENTITY_H
