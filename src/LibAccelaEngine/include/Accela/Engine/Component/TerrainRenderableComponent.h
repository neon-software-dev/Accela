/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_TERRAINRENDERABLECOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_TERRAINRENDERABLECOMPONENT_H

#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>

#include <string>

namespace Accela::Engine
{
    /**
     * Allows for a height-mapped terrain render to be attached to an entity
     *
     * Note: This is a different path for terrain generation than via using
     * IWorldResources::GenerateHeightMapMesh. This path uses tesselation shaders
     * to deform geometry in the pipeline, and has no physics associated with
     * the generated terrain.
     */
    struct TerrainRenderableComponent
    {
        /** The scene the terrain belongs to */
        std::string sceneName = DEFAULT_SCENE;

        /** The id of the material to draw the terrain with */
        Render::MaterialId materialId{Render::INVALID_ID};

        /** The dimensions, in x/z world space, of the terrain's area */
        Render::USize size{0, 0};

        /** The id of the height map texture to be used */
        Render::TextureId heightMapTextureId{Render::INVALID_ID};

        /** The tesselation level/amount to use for the terrain */
        float tesselationLevel{0.0f};

        /** The displacement factor to use for the terrain's height */
        float displacementFactor{0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_TERRAINRENDERABLECOMPONENT_H
