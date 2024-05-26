#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H

#include "LoadedTexture.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Texture/Texture.h>
#include <Accela/Render/Texture/TextureView.h>
#include <Accela/Render/Texture/TextureSampler.h>

#include <Accela/Common/ImageData.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <future>
#include <string>
#include <vector>

namespace Accela::Render
{
    class ITextures
    {
        public:

            virtual ~ITextures() = default;

            virtual bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                                    VkQueue vkTransferQueue) = 0;
            virtual void Destroy() = 0;

            virtual bool CreateTextureEmpty(const Texture& texture,
                                            const std::vector<TextureView>& textureViews,
                                            const TextureSampler& textureSampler) = 0;
            virtual bool CreateTextureFilled(const Texture& texture,
                                             const std::vector<TextureView>& textureViews,
                                             const TextureSampler& textureSampler,
                                             bool generateMipMaps,
                                             std::promise<bool> resultPromise) = 0;
            virtual std::optional<LoadedTexture> GetTexture(TextureId textureId) = 0;
            virtual LoadedTexture GetMissingTexture() = 0;
            virtual LoadedTexture GetMissingCubeTexture() = 0;
            virtual void DestroyTexture(TextureId textureId, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_ITEXTURES_H
