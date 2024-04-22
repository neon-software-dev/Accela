/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/SpriteEntity.h>
#include <Accela/Engine/Component/Components.h>

#include <Accela/Engine/IEngineRuntime.h>

namespace Accela::Engine
{

SpriteEntity::Params& SpriteEntity::Params::WithTextureId(Render::TextureId _textureId) { textureId = _textureId; return *this; }
SpriteEntity::Params& SpriteEntity::Params::WithSourcePixelRect(const Render::URect& _srcPixelRect) { srcPixelRect = _srcPixelRect; return *this; }
SpriteEntity::Params& SpriteEntity::Params::WithVirtualSize(const Render::FSize& _dstVirtualSize) { dstVirtualSize = _dstVirtualSize; return *this; }
SpriteEntity::Params& SpriteEntity::Params::WithPosition(const glm::vec3& _position) { position = _position; return *this; }
SpriteEntity::Params& SpriteEntity::Params::WithScale(const glm::vec2& _scale) { scale = _scale; return *this; }
SpriteEntity::Params& SpriteEntity::Params::WithOrientation(const glm::quat& _orientation) { orientation = _orientation; return *this; }

SpriteEntity::UPtr SpriteEntity::Create(const IEngineRuntime::Ptr& engine,
                                        const Params& params,
                                        const std::string& sceneName)
{
    return std::make_unique<SpriteEntity>(
        ConstructTag{},
        engine,
        engine->GetWorldState()->CreateEntity(),
        sceneName,
        params
    );
}

SpriteEntity::SpriteEntity(SpriteEntity::ConstructTag,
                           IEngineRuntime::Ptr engine,
                           EntityId eid,
                           std::string sceneName,
                           const Params& params)
    : Entity(std::move(engine), std::move(sceneName))
    , m_eid(eid)
    , m_textureId(params.textureId)
    , m_srcPixelRect(params.srcPixelRect)
    , m_dstVirtualSize(params.dstVirtualSize)
    , m_position(params.position)
    , m_scale(params.scale)
    , m_orientation(params.orientation)
{
    SyncAll();
}

SpriteEntity::~SpriteEntity()
{
    DestroyInternal();
}

void SpriteEntity::Destroy()
{
    DestroyInternal();
}

std::optional<EntityId> SpriteEntity::GetEid() const
{
    return m_eid;
}

void SpriteEntity::DestroyInternal()
{
    if (m_eid.has_value())
    {
        m_engine->GetWorldState()->DestroyEntity(*m_eid);
        m_eid = std::nullopt;
    }

    m_textureId = std::nullopt;
    m_srcPixelRect = std::nullopt;
    m_dstVirtualSize = std::nullopt;
    m_position = std::nullopt;
    m_scale = std::nullopt;
}

std::optional<Render::TextureId> SpriteEntity::GetTextureId() const noexcept
{
    return m_textureId;
}

void SpriteEntity::SetTextureById(Render::TextureId textureId)
{
    const auto dirty = !m_textureId || *m_textureId != textureId;

    m_textureId = textureId;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

bool SpriteEntity::SetTextureByAssetName(const std::string& assetName)
{
    const auto textureIdOpt = m_engine->GetWorldResources()->Textures()->GetAssetTextureId(assetName);
    if (!textureIdOpt)
    {
        return false;
    }

    SetTextureById(textureIdOpt.value());

    return true;
}

std::optional<Render::URect> SpriteEntity::GetSourcePixelRect() const noexcept
{
    return m_srcPixelRect;
}

void SpriteEntity::SetSourcePixelRect(const Render::URect& srcPixelRect)
{
    const auto dirty = !m_srcPixelRect || *m_srcPixelRect != srcPixelRect;

    m_srcPixelRect = srcPixelRect;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

std::optional<Render::FSize> SpriteEntity::GetDstVirtualSize() const noexcept
{
    return m_dstVirtualSize;
}

void SpriteEntity::SetDstVirtualSize(const Render::FSize& dstVirtualSize)
{
    const auto dirty = !m_dstVirtualSize || *m_dstVirtualSize != dstVirtualSize;

    m_dstVirtualSize = dstVirtualSize;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

std::optional<glm::vec3> SpriteEntity::GetPosition() const noexcept
{
    return m_position;
}

void SpriteEntity::SetPosition(const glm::vec3& position)
{
    const auto dirty = !m_position || *m_position != position;

    m_position = position;

    if (dirty && CanSyncTransformComponent()) { SyncTransformComponent(); }
}

std::optional<glm::vec2> SpriteEntity::GetScale() const noexcept
{
    return m_scale;
}

void SpriteEntity::SetScale(const glm::vec2& scale)
{
    const auto dirty = !m_scale || *m_scale != scale;

    m_scale = scale;

    if (dirty && CanSyncTransformComponent()) { SyncTransformComponent(); }
}

std::optional<glm::quat> SpriteEntity::GetOrientation() const noexcept
{
    return m_orientation;
}

void SpriteEntity::SetOrientation(const glm::quat& orientation)
{
    const auto dirty = !m_orientation || *m_orientation != orientation;

    m_orientation = orientation;

    if (dirty && CanSyncTransformComponent()) { SyncTransformComponent(); }
}

void SpriteEntity::SyncAll()
{
    if (CanSyncSpriteComponent()) { SyncSpriteComponent(); }
    if (CanSyncTransformComponent()) { SyncTransformComponent(); }
}

bool SpriteEntity::CanSyncSpriteComponent() const
{
    if (!m_eid) { return false; }
    if (!m_textureId) { return false; }

    return true;
}

void SpriteEntity::SyncSpriteComponent()
{
    auto spriteRenderableComponent = Engine::SpriteRenderableComponent{};
    spriteRenderableComponent.sceneName = m_sceneName;
    spriteRenderableComponent.textureId = *m_textureId;
    if (m_srcPixelRect) { spriteRenderableComponent.srcPixelRect = *m_srcPixelRect; }
    if (m_dstVirtualSize) { spriteRenderableComponent.dstVirtualSize = *m_dstVirtualSize; }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, spriteRenderableComponent);
}

bool SpriteEntity::CanSyncTransformComponent() const
{
    if (!m_eid) { return false; }
    if (!m_position) { return false; }

    return true;
}

void SpriteEntity::SyncTransformComponent()
{
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(*m_position);
    if (m_scale) { transformComponent.SetScale(glm::vec3(*m_scale, 1.0f)); }
    if (m_orientation) { transformComponent.SetOrientation(*m_orientation); }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);
}

}
