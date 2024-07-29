/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IMAGESAMPLER_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IMAGESAMPLER_H

#include <string>

namespace Accela::Render
{
    using ImageSamplerName = std::string;

    struct ImageSampler
    {
        static ImageViewName DEFAULT() { return "DEFAULT"; };
        static ImageViewName NEAREST() { return "NEAREST"; };

        ImageSamplerName name{DEFAULT()};
        VkFilter vkMagFilter{VK_FILTER_LINEAR};
        VkFilter vkMinFilter{VK_FILTER_LINEAR};
        VkSamplerAddressMode vkSamplerAddressModeU{VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
        VkSamplerAddressMode vkSamplerAddressModeV{VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
        VkSamplerMipmapMode vkSamplerMipmapMode{VK_SAMPLER_MIPMAP_MODE_LINEAR};

        VkBorderColor vkBorderColor{VK_BORDER_COLOR_INT_OPAQUE_BLACK};
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IMAGESAMPLER_H
