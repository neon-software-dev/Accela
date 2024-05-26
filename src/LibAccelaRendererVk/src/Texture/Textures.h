#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H

#include "ITextures.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <expected>
#include <unordered_map>
#include <unordered_set>

namespace Accela::Render
{
    class Textures : public ITextures
    {
        public:

            Textures(Common::ILogger::Ptr logger,
                     Common::IMetrics::Ptr metrics,
                     VulkanObjsPtr vulkanObjs,
                     IBuffersPtr buffers,
                     PostExecutionOpsPtr postExecutionOps,
                     Ids::Ptr ids);

            bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                            VkQueue vkTransferQueue) override;
            void Destroy() override;

            bool CreateTextureEmpty(const Texture& texture,
                                    const std::vector<TextureView>& textureViews,
                                    const TextureSampler& textureSampler) override;
            bool CreateTextureFilled(const Texture& texture,
                                     const std::vector<TextureView>& textureViews,
                                     const TextureSampler& textureSampler,
                                     bool generateMipMaps,
                                     std::promise<bool> resultPromise) override;
            std::optional<LoadedTexture> GetTexture(TextureId textureId) override;
            LoadedTexture GetMissingTexture() override;
            LoadedTexture GetMissingCubeTexture() override;
            void DestroyTexture(TextureId textureId, bool destroyImmediately) override;

        private:

            bool CreateMissingTexture();

            /**
             * Create a VkImage, VkImageView, and VkSampler for the provided texture spec
             */
            std::expected<LoadedTexture, bool> CreateTextureObjects(const Texture& texture,
                                                                    const std::vector<TextureView>& textureViews,
                                                                    const TextureSampler& textureSampler,
                                                                    const uint32_t& mipLevels,
                                                                    bool generatingMipMaps);

            [[nodiscard]] bool CreateTextureImage(LoadedTexture& loadedTexture, const Texture& texture, bool generatingMipMaps) const;
            [[nodiscard]] bool CreateTextureImageView(LoadedTexture& loadedTexture, const TextureView& textureView) const;
            [[nodiscard]] bool CreateTextureImageSampler(LoadedTexture& loadedTexture, const TextureSampler& textureSampler) const;

            void DestroyTextureObjects(const LoadedTexture& texture) const;

            [[nodiscard]] std::expected<VkFormat, bool> GetTextureImageFormat(const Texture& texture);
            [[nodiscard]] bool DoesDeviceSupportMipMapGeneration(const VkFormat& vkFormat) const;

            /**
             * Initiates an asynchronous data transfer of the provided image's data to the provided texture. Also
             * generates mipmaps as requested.
             */
            [[nodiscard]] bool TransferImageData(const LoadedTexture& loadedTexture,
                                                 const Common::ImageData::Ptr& imageData,
                                                 const uint32_t& mipLevels,
                                                 bool generateMipMaps,
                                                 bool initialDataTransfer,
                                                 std::promise<bool> resultPromise);

            [[nodiscard]] bool OnTextureTransferFinished(bool commandsSuccessful,
                                                         const LoadedTexture& loadedTexture,
                                                         bool initialDataTransfer);

            void SyncMetrics();

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            IBuffersPtr m_buffers;
            PostExecutionOpsPtr m_postExecutionOps;
            Ids::Ptr m_ids;

            VulkanCommandPoolPtr m_transferCommandPool;
            VkQueue m_vkTransferQueue{VK_NULL_HANDLE};

            TextureId m_missingTextureId{INVALID_ID};
            TextureId m_missingCubeTextureId{INVALID_ID};

            std::unordered_map<TextureId, LoadedTexture> m_textures;
            std::unordered_set<TextureId> m_texturesLoading;
            std::unordered_set<TextureId> m_texturesToDestroy;
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H
