/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include <Accela/Engine/Entity/CommandEntryEntity.h>

#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Component/Components.h>

#include <format>

namespace Accela::Engine
{

static const std::string PROMPT = "> ";

CommandEntryEntity::UPtr CommandEntryEntity::Create(const std::shared_ptr<IEngineRuntime>& engine,
                                                    const Platform::TextProperties& textProperties,
                                                    const std::string& sceneName)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    return std::make_unique<CommandEntryEntity>(ConstructTag{}, engine, textProperties, sceneName, eid);
}

CommandEntryEntity::CommandEntryEntity(ConstructTag, std::shared_ptr<IEngineRuntime> engine,
                                       Platform::TextProperties textProperties,
                                       std::string sceneName,
                                       EntityId eid)
    : Entity(std::move(engine), std::move(sceneName))
    , m_textProperties(std::move(textProperties))
    , m_eid(eid)
{
    SyncText();
}

CommandEntryEntity::~CommandEntryEntity()
{
    DestroyInternal();
}

void CommandEntryEntity::Destroy()
{
    DestroyInternal();
}

void CommandEntryEntity::DestroyInternal()
{
    if (m_eid.has_value())
    {
        m_engine->GetWorldState()->DestroyEntity(*m_eid);
        m_eid = std::nullopt;
    }

    if (m_textRender)
    {
        m_engine->GetWorldResources()->Textures()->DestroyTexture(m_textRender->textureId);
        m_textRender = std::nullopt;
    }
}

std::string CommandEntryEntity::GetEntry() const noexcept
{
    return m_entry;
}

void CommandEntryEntity::SetEntry(const std::string& entry)
{
    m_entry = entry;
    (void)SyncText();
}

void CommandEntryEntity::AppendToEntry(const std::string& text)
{
    m_entry += text;
    (void)SyncText();
}

void CommandEntryEntity::ClearEntry()
{
    m_entry = {};
    (void)SyncText();
}

void CommandEntryEntity::DeleteLastEntryChar()
{
    if (!m_entry.empty())
    {
        m_entry = m_entry.substr(0, m_entry.size() - 1);
    }
    (void)SyncText();
}

bool CommandEntryEntity::SyncText()
{
    if (!m_eid) { return false; }

    //
    // Text Component
    //
    std::string text = std::format("{}{}", PROMPT, m_entry);

    const auto textRender = m_engine->GetWorldResources()->Textures()->RenderText(text, m_textProperties, Engine::ResultWhen::Ready).get();
    if (!textRender) { return false; }

    // Destroy the old texture, if any
    if (m_textRender)
    {
        m_engine->GetWorldResources()->Textures()->DestroyTexture(m_textRender->textureId);
    }

    m_textRender = *textRender;

    auto spriteRenderableComponent = Engine::SpriteRenderableComponent{};
    spriteRenderableComponent.sceneName = m_sceneName;
    spriteRenderableComponent.textureId = m_textRender->textureId;
    spriteRenderableComponent.srcPixelRect = Render::URect(m_textRender->textPixelWidth, m_textRender->textPixelHeight);
    spriteRenderableComponent.dstVirtualSize = Render::FSize((float)m_textRender->textPixelWidth, (float)m_textRender->textPixelHeight);

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, spriteRenderableComponent);

    //
    // Transform Component
    //
    auto transformComponent = Engine::TransformComponent{};

    const auto offset = glm::vec3((float)m_textRender->textPixelWidth / 2.0f, (float)m_textRender->textPixelHeight / 2.0f, 0.0f);

    transformComponent.SetPosition(offset);

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);

    return true;
}

}
