#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H

#include <utility>

namespace Accela::Render
{
    enum class SamplerAddressMode
    {
        Wrap,
        Clamp,
        Mirror
    };

    using UVAddressMode = std::pair<SamplerAddressMode, SamplerAddressMode>;
    static constexpr UVAddressMode WRAP_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Wrap, SamplerAddressMode::Wrap);
    static constexpr UVAddressMode CLAMP_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Clamp, SamplerAddressMode::Clamp);
    static constexpr UVAddressMode MIRROR_ADDRESS_MODE = std::make_pair(SamplerAddressMode::Mirror, SamplerAddressMode::Mirror);

    struct TextureSampler
    {
        explicit TextureSampler(UVAddressMode _uvAddressMode)
            : uvAddressMode(std::move(_uvAddressMode))
        { }

        UVAddressMode uvAddressMode;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTURESAMPLER_H
