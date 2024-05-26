/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H

#include <cstdint>
#include <vector>
#include <string>

namespace Accela::Render
{
    struct TextureView
    {
        static constexpr std::string DEFAULT{"DEFAULT"};

        enum class ViewType
        {
            VIEW_TYPE_1D,
            VIEW_TYPE_2D,
            VIEW_TYPE_3D,
            VIEW_TYPE_CUBE,
            VIEW_TYPE_1D_ARRAY,
            VIEW_TYPE_2D_ARRAY,
            VIEW_TYPE_CUBE_ARRAY,
        };

        enum class Aspect
        {
            ASPECT_COLOR_BIT,
            ASPECT_DEPTH_BIT,
            ASPECT_STENCIL_BIT,
        };

        struct Layer
        {
            Layer(const uint32_t& _baseLayer, const uint32_t& _layerCount)
              : baseLayer(_baseLayer)
              , layerCount(_layerCount)
            { }

            uint32_t baseLayer;
            uint32_t layerCount;
        };

        //
        //
        //

        static TextureView ViewAs2D(std::string_view name, const Aspect& aspect)
        {
            return {name,
                    ViewType::VIEW_TYPE_2D,
                    {aspect},
                    Layer(0,1)
            };
        }

        static TextureView ViewAs2DArray(std::string_view name, const Aspect& aspect, const Layer& layers)
        {
            return {name,
                    ViewType::VIEW_TYPE_2D_ARRAY,
                    {aspect},
                    layers
            };
        }

        static TextureView ViewAsCube(std::string_view name, const Aspect& aspect)
        {
            return {name,
                    ViewType::VIEW_TYPE_CUBE,
                    {aspect},
                    Layer(0,6)
            };
        }

        TextureView() = default;

        TextureView(std::string_view _name,
                    const ViewType& _viewType,
                    const std::vector<Aspect>& _aspects,
                    const Layer& _layer)
            : name(_name)
            , viewType(_viewType)
            , aspects(_aspects)
            , layer(_layer)
        { }

        std::string name;
        ViewType viewType{ViewType::VIEW_TYPE_2D};
        std::vector<Aspect> aspects;
        Layer layer{0,1};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H
