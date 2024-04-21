/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_WORLDUPDATE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_WORLDUPDATE_H

#include "../Renderable/SpriteRenderable.h"
#include "../Renderable/ObjectRenderable.h"
#include "../Renderable/TerrainRenderable.h"

#include <Accela/Render/Light.h>

#include <vector>

namespace Accela::Render
{
    /**
     * Contains a list of changes that should be applied to the render state.
     *
     * Note: Any changes in here should also be reflected in HasAnyUpdate() at
     * the bottom of the file.
     */
    struct WorldUpdate
    {
        //
        // Changes to SpriteRenderables
        //
        std::vector<SpriteRenderable> toAddSpriteRenderables;
        std::vector<SpriteRenderable> toUpdateSpriteRenderables;
        std::vector<SpriteId> toDeleteSpriteIds;

        //
        // Changes to ObjectRenderables
        //
        std::vector<ObjectRenderable> toAddObjectRenderables;
        std::vector<ObjectRenderable> toUpdateObjectRenderables;
        std::vector<ObjectId> toDeleteObjectIds;

        //
        // Changes to TerrainRenderables
        //
        std::vector<TerrainRenderable> toAddTerrainRenderables;
        std::vector<TerrainRenderable> toUpdateTerrainRenderables;
        std::vector<TerrainId> toDeleteTerrainIds;

        //
        // Changes to 3D world lights
        //
        std::vector<Light> toAddLights;
        std::vector<Light> toUpdateLights;
        std::vector<LightId> toDeleteLightIds;

        [[nodiscard]] bool HasAnyUpdate() const noexcept
        {
            return
                !toAddSpriteRenderables.empty() ||
                !toUpdateSpriteRenderables.empty() ||
                !toDeleteSpriteIds.empty() ||

                !toAddObjectRenderables.empty() ||
                !toUpdateObjectRenderables.empty() ||
                !toDeleteObjectIds.empty() ||

                !toAddTerrainRenderables.empty() ||
                !toUpdateTerrainRenderables.empty() ||
                !toDeleteTerrainIds.empty() ||

                !toAddLights.empty() ||
                !toUpdateLights.empty() ||
                !toDeleteLightIds.empty();
        }
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_WORLDUPDATE_H
