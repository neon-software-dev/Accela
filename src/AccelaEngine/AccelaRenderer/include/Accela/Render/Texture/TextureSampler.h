/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H

#include <Accela/Common/SharedLib.h>

#include <utility>
#include <string>

namespace Accela::Render
{
    using TextureSamplerName = std::string;

    enum class SamplerAddressMode
    {
        Wrap,
        Clamp,
        Mirror
    };

    enum class SamplerFilterMode
    {
        Nearest,
        Linear
    };

    using UVAddressMode = std::pair<SamplerAddressMode, SamplerAddressMode>;
    static constexpr UVAddressMode WRAP_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Wrap, SamplerAddressMode::Wrap);
    static constexpr UVAddressMode CLAMP_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Clamp, SamplerAddressMode::Clamp);
    static constexpr UVAddressMode MIRROR_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Mirror, SamplerAddressMode::Mirror);

    struct ACCELA_PUBLIC TextureSampler
    {
        static TextureSamplerName DEFAULT() { return "DEFAULT"; };
        static TextureSamplerName NEAREST() { return "NEAREST"; };

        explicit TextureSampler(TextureSamplerName _name, UVAddressMode _uvAddressMode)
            : name(std::move(_name))
            , uvAddressMode(_uvAddressMode)
        { }

        TextureSamplerName name;
        UVAddressMode uvAddressMode;
        SamplerFilterMode minFilter{SamplerFilterMode::Linear};
        SamplerFilterMode magFilter{SamplerFilterMode::Linear};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
