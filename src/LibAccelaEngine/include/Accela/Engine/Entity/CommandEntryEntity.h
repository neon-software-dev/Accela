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
                                                   const std::string& sceneName = DEFAULT_SCENE);

            CommandEntryEntity(ConstructTag,
                               std::shared_ptr<IEngineRuntime> engine,
                               Platform::TextProperties textProperties,
                               std::string sceneName,
                               EntityId eid);
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

            std::optional<TextRender> m_textRender;
            std::string m_entry;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_COMMANDENTRYENTITY_H
