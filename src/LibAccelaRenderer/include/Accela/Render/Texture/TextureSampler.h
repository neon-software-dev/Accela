/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H

#include <utility>
#include <string>

namespace Accela::Render
{
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

    struct TextureSampler
    {
        static constexpr std::string DEFAULT{"DEFAULT"};
        static constexpr std::string NEAREST{"NEAREST"};

        explicit TextureSampler(std::string_view _name, UVAddressMode _uvAddressMode)
            : name(_name)
            , uvAddressMode(std::move(_uvAddressMode))
        { }

        std::string name;
        UVAddressMode uvAddressMode;
        SamplerFilterMode minFilter{SamplerFilterMode::Linear};
        SamplerFilterMode magFilter{SamplerFilterMode::Linear};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
