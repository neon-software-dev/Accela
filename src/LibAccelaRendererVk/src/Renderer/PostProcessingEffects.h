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

    [[nodiscard]] static PostProcessEffect ToneMappingEffect(const RenderSettings& renderSettings)
    {
        ToneMappingPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;
        pushPayload.hdr_exposure = renderSettings.exposure;

        std::vector<std::byte> pushPayloadBytes(sizeof(ToneMappingPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(ToneMappingPushPayload));

        return {
            .programName = "ToneMapping",
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

    [[nodiscard]] static PostProcessEffect GammaCorrectionEffect(const RenderSettings& renderSettings)
    {
        GammaCorrectionPushPayload pushPayload{};
        pushPayload.renderWidth = renderSettings.resolution.w;
        pushPayload.renderHeight = renderSettings.resolution.h;
        pushPayload.gamma = renderSettings.gamma;

        std::vector<std::byte> pushPayloadBytes(sizeof(GammaCorrectionPushPayload));
        memcpy(pushPayloadBytes.data(), &pushPayload, sizeof(GammaCorrectionPushPayload));

        return {
            .programName = "GammaCorrection",
            .pushPayload = pushPayloadBytes,
            .tag = "GammaCorrection"
        };
    }
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGEFFECTS_H
