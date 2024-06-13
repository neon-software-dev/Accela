/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_HEIGHTMAPDATA_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_HEIGHTMAPDATA_H

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
        std::vector<float> data;

        Render::USize dataSize;
        float minValue{0.0f};
        float maxValue{0.0f};

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

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_HEIGHTMAPDATA_H
