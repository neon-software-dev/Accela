/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H

#include <Accela/Common/Id.h>

namespace Accela::Render
{
    DEFINE_ID_TYPE(TextureId)
    DEFINE_ID_TYPE(FrameBufferId)
    DEFINE_ID_TYPE(MeshId)
    DEFINE_ID_TYPE(RenderableId)
    DEFINE_ID_TYPE(SpriteId)
    DEFINE_ID_TYPE(ObjectId)
    DEFINE_ID_TYPE(TerrainId)
    DEFINE_ID_TYPE(MaterialId)
    DEFINE_ID_TYPE(LightId)
    DEFINE_ID_TYPE(RenderTargetId)
}

DEFINE_ID_HASH(Accela::Render::TextureId)
DEFINE_ID_HASH(Accela::Render::FrameBufferId)
DEFINE_ID_HASH(Accela::Render::MeshId)
DEFINE_ID_HASH(Accela::Render::RenderableId)
DEFINE_ID_HASH(Accela::Render::SpriteId)
DEFINE_ID_HASH(Accela::Render::ObjectId)
DEFINE_ID_HASH(Accela::Render::TerrainId)
DEFINE_ID_HASH(Accela::Render::MaterialId)
DEFINE_ID_HASH(Accela::Render::LightId)
DEFINE_ID_HASH(Accela::Render::RenderTargetId)

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H
