/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H

#include "IdSource.h"

#include <Accela/Common/SharedLib.h>

#include <memory>

namespace Accela::Render
{
    /**
     * Provides IdSources for all the Renderer Id types
     */
    struct ACCELA_PUBLIC Ids
    {
        using Ptr = std::shared_ptr<Ids>;

        IdSource<TextureId> textureIds;
        IdSource<FrameBufferId> frameBufferIds;
        IdSource<MeshId> meshIds;
        IdSource<SpriteId> spriteIds;
        IdSource<ObjectId> objectIds;
        IdSource<TerrainId> terrainIds;
        IdSource<MaterialId> materialIds;
        IdSource<LightId> lightIds;
        IdSource<RenderTargetId> renderTargetIds;

        void Reset()
        {
            textureIds.Reset();
            frameBufferIds.Reset();
            meshIds.Reset();
            spriteIds.Reset();
            objectIds.Reset();
            terrainIds.Reset();
            materialIds.Reset();
            lightIds.Reset();
            renderTargetIds.Reset();
        }
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H
