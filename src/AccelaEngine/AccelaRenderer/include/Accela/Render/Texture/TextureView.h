/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H

#include <Accela/Common/SharedLib.h>

#include <cstdint>
#include <vector>
#include <string>

namespace Accela::Render
{
    using TextureViewName = std::string;

    struct ACCELA_PUBLIC TextureView
    {
        static TextureViewName DEFAULT() { return "DEFAULT"; };

        enum class ViewType
        {
            VIEW_TYPE_2D,
            VIEW_TYPE_2D_ARRAY,
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

        static TextureView ViewAs2D(const TextureViewName& name)
        {
            return {name, ViewType::VIEW_TYPE_2D, Layer(0,1)
            };
        }

        static TextureView ViewAs2DArray(const TextureViewName& name, const Layer& layer)
        {
            return {name, ViewType::VIEW_TYPE_2D_ARRAY, layer
            };
        }

        static TextureView ViewAsCube(const TextureViewName& name)
        {
            return {name, ViewType::VIEW_TYPE_CUBE, Layer(0,6)
            };
        }

        TextureView()
            : name(DEFAULT())
        { }

        TextureView(TextureViewName _name, const ViewType& _viewType, const Layer& _layer)
            : name(std::move(_name))
            , viewType(_viewType)
            , layer(_layer)
        { }

        TextureViewName name;
        ViewType viewType{ViewType::VIEW_TYPE_2D};
        Layer layer{0,1};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREVIEW_H
