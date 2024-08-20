/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGET_H
#define LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGET_H

#include "../InternalId.h"

#include <Accela/Render/Id.h>

#include <string>

namespace Accela::Render
{
    struct RenderTarget
    {
        explicit RenderTarget(std::string _tag)
            : tag(std::move(_tag))
        { }

        RenderTarget(FrameBufferId _gPassFramebuffer,
                     FrameBufferId _screenFramebuffer,
                     ImageId _postProcessOutputImage,
                     std::string _tag)
            : gPassFramebuffer(_gPassFramebuffer)
            , screenFramebuffer(_screenFramebuffer)
            , postProcessOutputImage(_postProcessOutputImage)
            , tag(std::move(_tag))
        {}

        FrameBufferId gPassFramebuffer{};
        FrameBufferId screenFramebuffer{};
        ImageId postProcessOutputImage{};

        std::string tag;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGET_H
