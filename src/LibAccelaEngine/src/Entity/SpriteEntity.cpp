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
                           Params params)
    : Entity(std::move(engine), std::move(sceneName))
    , m_eid(eid)
    , m_params(std::move(params))
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

    m_params = std::nullopt;
}

std::optional<Render::TextureId> SpriteEntity::GetTextureId() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->textureId;
}

void SpriteEntity::SetTextureById(Render::TextureId textureId)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->textureId || *m_params->textureId != textureId;

    m_params->textureId = textureId;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

bool SpriteEntity::SetTextureByResource(const ResourceIdentifier& resource)
{
    const auto textureIdOpt = m_engine->GetWorldResources()->Textures()->GetTextureId(resource);
    if (!textureIdOpt)
    {
        return false;
    }

    SetTextureById(textureIdOpt.value());

    return true;
}

std::optional<Render::URect> SpriteEntity::GetSourcePixelRect() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->srcPixelRect;
}

void SpriteEntity::SetSourcePixelRect(const Render::URect& srcPixelRect)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->srcPixelRect || *m_params->srcPixelRect != srcPixelRect;

    m_params->srcPixelRect = srcPixelRect;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

std::optional<Render::FSize> SpriteEntity::GetDstVirtualSize() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->dstVirtualSize;
}

void SpriteEntity::SetDstVirtualSize(const Render::FSize& dstVirtualSize)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->dstVirtualSize || *m_params->dstVirtualSize != dstVirtualSize;

    m_params->dstVirtualSize = dstVirtualSize;

    if (dirty && CanSyncSpriteComponent()) { SyncSpriteComponent(); }
}

std::optional<glm::vec3> SpriteEntity::GetPosition() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->position;
}

void SpriteEntity::SetPosition(const glm::vec3& position)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->position || *m_params->position != position;

    m_params->position = position;

    if (dirty && CanSyncTransformComponent()) { SyncTransformComponent(); }
}

std::optional<glm::vec2> SpriteEntity::GetScale() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->scale;
}

void SpriteEntity::SetScale(const glm::vec2& scale)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->scale || *m_params->scale != scale;

    m_params->scale = scale;

    if (dirty && CanSyncTransformComponent()) { SyncTransformComponent(); }
}

std::optional<glm::quat> SpriteEntity::GetOrientation() const noexcept
{
    if (!m_params) { return std::nullopt; }
    return m_params->orientation;
}

void SpriteEntity::SetOrientation(const glm::quat& orientation)
{
    if (!m_params) { return; }

    const auto dirty = !m_params->orientation || *m_params->orientation != orientation;

    m_params->orientation = orientation;

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
    if (!m_params->textureId) { return false; }

    return true;
}

void SpriteEntity::SyncSpriteComponent()
{
    auto spriteRenderableComponent = Engine::SpriteRenderableComponent{};
    spriteRenderableComponent.sceneName = m_sceneName;
    spriteRenderableComponent.textureId = *m_params->textureId;
    if (m_params->srcPixelRect) { spriteRenderableComponent.srcPixelRect = *m_params->srcPixelRect; }
    if (m_params->dstVirtualSize) { spriteRenderableComponent.dstVirtualSize = *m_params->dstVirtualSize; }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, spriteRenderableComponent);
}

bool SpriteEntity::CanSyncTransformComponent() const
{
    if (!m_eid) { return false; }
    if (!m_params->position) { return false; }

    return true;
}

void SpriteEntity::SyncTransformComponent()
{
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(*m_params->position);
    if (m_params->scale) { transformComponent.SetScale(glm::vec3(*m_params->scale, 1.0f)); }
    if (m_params->orientation) { transformComponent.SetOrientation(*m_params->orientation); }

    Engine::AddOrUpdateComponent(m_engine->GetWorldState(), *m_eid, transformComponent);
}

}
