/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_IMAGE_IMAGEDEFINITION_H
#define LIBACCELARENDERERVK_SRC_IMAGE_IMAGEDEFINITION_H

#include "Image.h"
#include "ImageView.h"
#include "ImageSampler.h"

#include <vector>

namespace Accela::Render
{
    struct ImageDefinition
    {
        Image image;
        std::vector<ImageView> imageViews;
        std::vector<ImageSampler> imageSamplers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_IMAGE_IMAGEDEFINITION_H
