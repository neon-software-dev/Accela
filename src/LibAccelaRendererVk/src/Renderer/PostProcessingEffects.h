/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGEFFECTS_H
#define LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGEFFECTS_H

#include "PostProcessingRenderer.h"

namespace Accela::Render
{
    //
    // Tone Mapping Effect
    //
    struct ToneMappingPushPayload
    {
        // Required
        alignas(4) uint32_t renderWidth{0};
        alignas(4) uint32_t renderHeight{0};

        // Effect-specific
        alignas(4) float hdr_exposure{1.0f};
    };

    [[nodiscard]] [[maybe_unused]] static PostProcessEffect ToneMappingEffect(const RenderSettings& renderSettings)
    {
        ToneMappingPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;
        pushPayload.hdr_exposure = renderSettings.exposure;

        std::vector<std::byte> pushPayloadBytes(sizeof(ToneMappingPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(ToneMappingPushPayload));

        return {
            .programName = "ToneMapping",
            .inputImageView = ImageView::DEFAULT,
            .inputImageSampler = ImageSampler::NEAREST,
            .additionalSamplers = {},
            .bufferPayloads = {},
            .pushPayload = pushPayloadBytes,
            .tag = "ToneMapping"
        };
    }

    //
    // Gamma Correction Effect
    //
    struct GammaCorrectionPushPayload
    {
        // Required
        alignas(4) uint32_t renderWidth{0};
        alignas(4) uint32_t renderHeight{0};

        // Effect-specific
        alignas(4) float gamma{1.0f};
    };

    [[nodiscard]] [[maybe_unused]] static PostProcessEffect GammaCorrectionEffect(const RenderSettings& renderSettings)
    {
        GammaCorrectionPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;
        pushPayload.gamma = renderSettings.gamma;

        std::vector<std::byte> pushPayloadBytes(sizeof(GammaCorrectionPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(GammaCorrectionPushPayload));

        return {
            .programName = "GammaCorrection",
            .inputImageView = ImageView::DEFAULT,
            .inputImageSampler = ImageSampler::NEAREST,
            .additionalSamplers = {},
            .bufferPayloads = {},
            .pushPayload = pushPayloadBytes,
            .tag = "GammaCorrection"
        };
    }

    //
    // FXAA Effect
    //
    struct FXAAPushPayload
    {
        // Required
        alignas(4) uint32_t renderWidth{0};
        alignas(4) uint32_t renderHeight{0};
    };

    [[nodiscard]] [[maybe_unused]] static PostProcessEffect FXAAEffect(const RenderSettings& renderSettings)
    {
        FXAAPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;

        std::vector<std::byte> pushPayloadBytes(sizeof(FXAAPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(FXAAPushPayload));

        return {
            .programName = "FXAA",
            .inputImageView = ImageView::DEFAULT,
            .inputImageSampler = ImageSampler::DEFAULT,
            .additionalSamplers = {},
            .bufferPayloads = {},
            .pushPayload = pushPayloadBytes,
            .tag = "FXAA"
        };
    }

    //
    // ObjectHighlight Effect
    //
    struct ObjectHighlightPushPayload
    {
        // Required
        alignas(4) uint32_t renderWidth{0};
        alignas(4) uint32_t renderHeight{0};

        // Effect-specific
        alignas(4) uint32_t highlightMode{0};
        alignas(16) glm::vec3 highlightColor{0};
        alignas(4) uint32_t numHighlightedObjects{0};
    };

    [[nodiscard]] [[maybe_unused]] static PostProcessEffect ObjectHighlightEffect(const RenderSettings& renderSettings,
                                                                                  const LoadedImage& objectDetailImage,
                                                                                  const std::unordered_set<ObjectId>& highlightedObjects)
    {
        //
        // Highlighted Objects Input Buffer
        //
        std::vector<IdType> highlightedObjectIds;

        std::ranges::transform(highlightedObjects, std::back_inserter(highlightedObjectIds), [](const auto& objectId){
            return objectId.id;
        });

        // Special-case create at least one unused id entry so that we don't create a zero-sized buffer
        if (highlightedObjectIds.empty())
        {
            highlightedObjectIds.push_back(INVALID_ID);
        }

        const auto highlightedObjectsPayloadSize = highlightedObjectIds.size() * sizeof(IdType);

        std::vector<std::byte> highlightedObjectsPayload(highlightedObjectsPayloadSize);
        memcpy(highlightedObjectsPayload.data(), highlightedObjectIds.data(), highlightedObjectsPayloadSize);

        //
        // Highlighted Objects Push Payload
        //
        ObjectHighlightPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;
        pushPayload.highlightMode = static_cast<uint32_t>(renderSettings.highlightMode);
        pushPayload.highlightColor = renderSettings.highlightColor;
        pushPayload.numHighlightedObjects = highlightedObjectIds.size();

        std::vector<std::byte> pushPayloadBytes(sizeof(ObjectHighlightPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(ObjectHighlightPushPayload));

        return {
            .programName = "ObjectHighlight",
            .inputImageView = ImageView::DEFAULT,
            .inputImageSampler = ImageSampler::DEFAULT,
            .additionalSamplers = {
                {"i_objectDetail", objectDetailImage, VK_IMAGE_ASPECT_COLOR_BIT, ImageView::DEFAULT, ImageSampler::NEAREST}
             },
            .bufferPayloads = {
                {"i_highlightedObjects", highlightedObjectsPayload}
             },
            .pushPayload = pushPayloadBytes,
            .tag = "ObjectHighlight"
        };
    }
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGEFFECTS_H
