/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H
#define LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H

#include <Accela/Render/Mesh/Mesh.h>
#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/ImageData.h>

#include <vector>
#include <memory>
#include <string>

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

    [[nodiscard]] HeightMapData::Ptr GenerateHeightMapData(const Common::ImageData::Ptr& heightMapImage,
                                                                  const Render::USize& heightMapDataSize,
                                                                  const Render::USize& meshSize_worldSpace,
                                                                  const float& displacementFactor);

    [[nodiscard]] Render::Mesh::Ptr GenerateHeightMapMesh(const Render::MeshId& id,
                                                                 const HeightMapData& heightMapData,
                                                                 const Render::USize& meshSize_worldSpace,
                                                                 const std::string& tag);
}

#endif //LIBACCELAENGINE_SRC_SCENE_HEIGHTMAPDATA_H
