/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H
#define LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H

#include "ITextures.h"

#include "../ForwardDeclares.h"

#include "../Image/ImageDefinition.h"

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
                     IImagesPtr images,
                     IBuffersPtr buffers,
                     PostExecutionOpsPtr postExecutionOps,
                     Ids::Ptr ids);

            bool Initialize() override;
            void Destroy() override;

            bool CreateTexture(const TextureDefinition& textureDefinition, std::promise<bool> resultPromise) override;
            std::optional<LoadedTexture> GetTexture(TextureId textureId) override;
            std::optional<std::pair<LoadedTexture, LoadedImage>> GetTextureAndImage(TextureId textureId) override;
            std::pair<LoadedTexture, LoadedImage> GetMissingTexture() override;
            std::pair<LoadedTexture, LoadedImage> GetMissingCubeTexture() override;
            bool UpdateTexture(TextureId textureId, const Common::ImageData::Ptr& imageData, std::promise<bool> resultPromise) override;
            void DestroyTexture(TextureId textureId, bool destroyImmediately) override;

        private:

            bool CreateMissingTexture();

            void SyncMetrics() const;

            [[nodiscard]] static ImageDefinition TextureDefToImageDef(const TextureDefinition& textureDefinition);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            IImagesPtr m_images;
            IBuffersPtr m_buffers;
            PostExecutionOpsPtr m_postExecutionOps;
            Ids::Ptr m_ids;

            TextureId m_missingTextureId{INVALID_ID};
            TextureId m_missingCubeTextureId{INVALID_ID};

            std::unordered_map<TextureId, LoadedTexture> m_textures;
    };
}

#endif //LIBACCELARENDERERVK_SRC_TEXTURE_TEXTURES_H
