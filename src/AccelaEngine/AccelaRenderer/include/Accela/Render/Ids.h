/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H

#include "Id.h"

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/IdSource.h>

#include <memory>

namespace Accela::Render
{
    /**
     * Provides IdSources for all the Renderer Id types
     */
    struct ACCELA_PUBLIC Ids
    {
        using Ptr = std::shared_ptr<Ids>;

        Common::IdSource<TextureId> textureIds;
        Common::IdSource<FrameBufferId> frameBufferIds;
        Common::IdSource<MeshId> meshIds;
        Common::IdSource<SpriteId> spriteIds;
        Common::IdSource<ObjectId> objectIds;
        Common::IdSource<TerrainId> terrainIds;
        Common::IdSource<MaterialId> materialIds;
        Common::IdSource<LightId> lightIds;
        Common::IdSource<RenderTargetId> renderTargetIds;

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
