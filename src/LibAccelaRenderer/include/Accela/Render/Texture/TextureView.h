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
    using TextureViewName = std::string;

    struct TextureView
    {
        static constexpr TextureViewName DEFAULT{"DEFAULT"};

        enum class ViewType
        {
            VIEW_TYPE_2D,
            VIEW_TYPE_CUBE
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

        static TextureView ViewAs2D(std::string_view name)
        {
            return {name,
                    ViewType::VIEW_TYPE_2D,
                    Layer(0,1)
            };
        }

        static TextureView ViewAsCube(std::string_view name)
        {
            return {name,
                    ViewType::VIEW_TYPE_CUBE,
                    Layer(0,6)
            };
        }

        TextureView() = default;

        TextureView(std::string_view _name, const ViewType& _viewType, const Layer& _layer)
            : name(_name)
            , viewType(_viewType)
            , layer(_layer)
        { }

        TextureViewName name;
        ViewType viewType{ViewType::VIEW_TYPE_2D};
        Layer layer{0,1};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H
