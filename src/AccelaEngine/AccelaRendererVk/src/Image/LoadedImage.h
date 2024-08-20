/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_LOADEDIMAGE_H
#define LIBACCELARENDERERVK_SRC_IMAGE_LOADEDIMAGE_H

#include "Image.h"
#include "ImageView.h"
#include "ImageSampler.h"

#include "../InternalId.h"

#include "../Util/ImageAllocation.h"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

namespace Accela::Render
{
    struct LoadedImage
    {
        LoadedImage() = default;

        LoadedImage(Image _image, ImageAllocation _allocation)
            : image(std::move(_image))
            , allocation(_allocation)
        { }

        ImageId id;
        Image image;
        ImageAllocation allocation;
        std::unordered_map<ImageViewName, VkImageView> vkImageViews;
        std::unordered_map<ImageViewName, VkSampler> vkSamplers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_LOADEDIMAGE_H
