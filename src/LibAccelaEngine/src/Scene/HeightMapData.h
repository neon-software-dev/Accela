/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H
#define LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H

#include <Accela/Render/Util/Rect.h>

#include <vector>
#include <memory>

namespace Accela::Engine
{
    struct HeightMapData
    {
        using Ptr = std::shared_ptr<HeightMapData>;

        // Height map data, with values of ((pixelValue[0..255]) / 255.0) * heightMapDisplacement
        //
        // Stored as doubles as we were losing too much precision and hitting asserts when rp3d was
        // internally doing height map math with floats.
        //
        // WARNING: Physics system colliders directly reference the vector's heap data, so if the
        // vector is ever changed the physics system has to be re-synced to the data before the next
        // physics simulation step runs.
        std::vector<double> data;

        Render::USize dataSize;
        double minValue{0.0f};
        double maxValue{0.0f};

        Render::USize meshSize_worldSpace;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H
