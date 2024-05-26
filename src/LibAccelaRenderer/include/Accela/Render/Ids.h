#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H

#include "IdSource.h"

#include <memory>

namespace Accela::Render
{
    /**
     * Provides IdSources for all the Renderer Id types
     */
    struct Ids
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
        }
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_IDS_H
