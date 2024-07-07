/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IMAGEVIEW_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IMAGEVIEW_H

#include <string>

namespace Accela::Render
{
    using ImageViewName = std::string;

    struct ImageView
    {
        static constexpr ImageViewName DEFAULT{"DEFAULT"};

        ImageViewName name{DEFAULT};
        VkImageViewType vkImageViewType{VK_IMAGE_VIEW_TYPE_2D};
        VkImageAspectFlags vkImageAspectFlags{VK_IMAGE_ASPECT_COLOR_BIT};
        uint32_t baseLayer{0};
        uint32_t layerCount{1};
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IMAGEVIEW_H
