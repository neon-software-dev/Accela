/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDHEIGHTMAP_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDHEIGHTMAP_H

#include "LoadedStaticMesh.h"

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC LoadedHeightMap
    {
        std::size_t dataWidth{0};
        std::size_t dataHeight{0};

        float minValue{0.0f};
        float maxValue{0.0f};

        float worldWidth{0.0f};
        float worldHeight{0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_LOADEDHEIGHTMAP_H
