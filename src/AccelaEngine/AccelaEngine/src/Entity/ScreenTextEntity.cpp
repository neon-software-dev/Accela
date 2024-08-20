/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/ScreenTextEntity.h>
#include <Accela/Engine/Component/Components.h>

#include <cassert>

namespace Accela::Engine
{

ScreenTextEntity::Params& ScreenTextEntity::Params::WithText(const std::string& _text) { text = _text; return *this; }
ScreenTextEntity::Params& ScreenTextEntity::Params::WithProperties(const Platform::TextProperties& _properties) { properties = _properties; return *this; }
ScreenTextEntity::Params& ScreenTextEntity::Params::WithTextLayoutMode(TextLayoutMode _textLayoutMode) { textLayoutMode = _textLayoutMode; return *this; }
ScreenTextEntity::Params& ScreenTextEntity::Params::WithPosition(const glm::vec3& _position) { position = _position; return *this; }

ScreenTextEntity::UPtr ScreenTextEntity::Create(const IEngineRuntime::Ptr& engine,
                                                const Params& params,
                                                const std::string& sceneName)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    return std::make_unique<ScreenTextEntity>(ConstructTag{}, engine, eid, sceneName, params);
}

ScreenTextEntity::ScreenTextEntity(ConstructTag,
                                   IEngineRuntime::Ptr engine,
                                   EntityId eid,
                                   std::string sceneName,
                                   const Params& params)
    : Entity(std::move(engine), std::move(sceneName))
    , m_eid(eid)
    , m_text(params.text)
    , m_properties(params.properties)
    , m_position(params.position)
{
    if (params.textLayoutMode)
    {
        m_textLayoutMode = *params.textLayoutMode;
    }

    SyncAll();
}

ScreenTextEntity::~ScreenTextEntity()
{
    DestroyInternal();
}

void ScreenTextEntity::Destroy()
{
    DestroyInternal();
}

void ScreenTextEntity::DestroyInternal()
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

std::optional<EntityId> ScreenTextEntity::GetEid() const
{
    return m_eid;
}

std::optional<std::string> ScreenTextEntity::GetText() const
{
    return m_text;
}

bool ScreenTextEntity::SetText(const std::string& text)
{
    bool dirty = false;

    if (!m_text || *m_text != text)
    {
        dirty = true;
    }

    m_text = text;

    if (dirty) { SyncAll(); }

    return true;
}

std::optional<Platform::TextProperties> ScreenTextEntity::GetTextProperties() const
{
    return m_properties;
}

bool ScreenTextEntity::SetTextProperties(const Platform::TextProperties& properties)
{
    bool dirty = false;

    if (!m_properties || *m_properties != properties)
    {
        dirty = true;
    }

    m_properties = properties;

    if (dirty) { SyncAll(); }

    return true;
}

TextLayoutMode ScreenTextEntity::GetTextLayoutMode() const
{
    return m_textLayoutMode;
}

bool ScreenTextEntity::SetTextLayoutMode(TextLayoutMode textLayoutMode)
{
    const auto dirty = m_textLayoutMode != textLayoutMode;

    m_textLayoutMode = textLayoutMode;

    if (dirty) { SyncAll(); }

    return true;
}

std::optional<glm::vec3> ScreenTextEntity::GetPosition() const
{
    return m_position;
}

bool ScreenTextEntity::SetPosition(const glm::vec3& position)
{
    const auto dirty = !m_position || *m_position != position;

    m_position = position;

    if (dirty) { SyncAll(); }

    return true;
}

std::optional<Render::USize> ScreenTextEntity::GetRenderedTextSize() const
{
    if (!m_textRender) { return std::nullopt; }

    return Render::USize(m_textRender->textPixelWidth, m_textRender->textPixelHeight);
}

void ScreenTextEntity::SyncAll()
{
    if (CanSyncText()) { SyncText(); }
    if (CanSyncPosition()) { SyncPosition(); }
}

bool ScreenTextEntity::CanSyncText()
{
    // Need to have text to render and text properties to render it with
    return m_text && m_properties;
}

bool ScreenTextEntity::SyncText()
{
    if (!m_eid) { return false; }
    if (!m_properties) { return false; }
    if (!m_text) { return false; }

    const auto textRender = m_engine->GetWorldResources()->Textures()->RenderText(*m_text, *m_properties, Engine::ResultWhen::Ready).get();
    if (!textRender) { return false; }

    // Destroy the old texture, if any
    if (m_textRender)
    {
        m_engine->GetWorldResources()->Textures()->DestroyTexture(m_textRender->textureId);
    }

    m_textRender = *textRender;

    const auto textRenderVirtualSize = m_engine->GetWorldState()->RenderSizeToVirtualSize(
        {m_textRender->textPixelWidth, m_textRender->textPixelHeight}
    );

    auto spriteRenderableComponent = Engine::SpriteRenderableComponent{};
    spriteRenderableComponent.sceneName = m_sceneName;
    spriteRenderableComponent.textureId = m_textRender->textureId;
    spriteRenderableComponent.srcPixelRect = Render::URect(m_textRender->textPixelWidth, m_textRender->textPixelHeight);
    spriteRenderableComponent.dstVirtualSize = Render::FSize((float)textRenderVirtualSize.w, (float)textRenderVirtualSize.h);

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, spriteRenderableComponent);

    return true;
}

bool ScreenTextEntity::CanSyncPosition()
{
    switch (m_textLayoutMode)
    {
        case TextLayoutMode::Center:
            // Only need to know which position to center the sprite at
            return m_position.has_value();
        case TextLayoutMode::TopLeft:
            // Need to know position as well as have rendered text in order to offset by the render's dimensions
            return m_position.has_value() && m_textRender.has_value();
    }

    assert(false);
    return false;
}

bool ScreenTextEntity::SyncPosition()
{
    if (!m_eid) { return false; }
    if (!m_position) { return false; }

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetScale(glm::vec3(1.0f));

    switch (m_textLayoutMode)
    {
        case TextLayoutMode::Center:
        {
            transformComponent.SetPosition(*m_position);
        }
        break;
        case TextLayoutMode::TopLeft:
        {
            if (!m_textRender) { return false; }

            const auto textRenderVirtualSize = m_engine->GetWorldState()->RenderSizeToVirtualSize(
                {m_textRender->textPixelWidth, m_textRender->textPixelHeight}
            );

            const auto offset = glm::vec3((float)textRenderVirtualSize.w / 2.0f, (float)textRenderVirtualSize.h / 2.0f, 0.0f);

            transformComponent.SetPosition(*m_position + offset);
        }
        break;
    }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);

    return true;
}

}
