/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H

#include "LoadedTexture.h"

#include "../ForwardDeclares.h"

#include "../Image/LoadedImage.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Texture/TextureDefinition.h>

#include <Accela/Common/ImageData.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <future>
#include <string>
#include <vector>
#include <utility>

namespace Accela::Render
{
    class ITextures
    {
        public:

            virtual ~ITextures() = default;

            virtual bool Initialize() = 0;
            virtual void Destroy() = 0;

            virtual bool CreateTexture(const TextureDefinition& textureDefinition, std::promise<bool> resultPromise) = 0;
            virtual std::optional<LoadedTexture> GetTexture(TextureId textureId) = 0;
            virtual std::optional<std::pair<LoadedTexture, LoadedImage>> GetTextureAndImage(TextureId textureId) = 0;
            virtual std::pair<LoadedTexture, LoadedImage> GetMissingTexture() = 0;
            virtual std::pair<LoadedTexture, LoadedImage> GetMissingCubeTexture() = 0;
            virtual void DestroyTexture(TextureId textureId, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H
