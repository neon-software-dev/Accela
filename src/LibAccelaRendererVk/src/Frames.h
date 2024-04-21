/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMES_H
#define LIBACCELARENDERERVK_SRC_FRAMES_H

#include "FrameState.h"
#include "ForwardDeclares.h"

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Accela::Render
{
    enum class SurfaceIssue
    {
        SurfaceInvalidated,
        SurfaceLost
    };

    class Frames
    {
        public:

            Frames(Common::ILogger::Ptr logger,
                   Ids::Ptr ids,
                   VulkanObjsPtr vulkanObjs,
                   ITexturesPtr textures);

            bool Initialize(const RenderSettings& renderSettings, const VulkanSwapChainPtr& swapChain);
            void Destroy();

            void OnSwapChainChanged(const VulkanSwapChainPtr& swapChain);
            bool OnRenderSettingsChanged(const RenderSettings& renderSettings);

            [[nodiscard]] std::expected<uint32_t, SurfaceIssue> StartFrame();
            FrameState& GetCurrentFrame();
            FrameState& GetNextFrame();
            void EndFrame();

        private:

            bool CreateFrames(const RenderSettings& renderSettings);

            void WaitForFrameWorkToFinish(uint32_t frameIndex);
            [[nodiscard]] std::expected<uint32_t, SurfaceIssue> AcquireNextSwapChainImageIndex();

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            VulkanObjsPtr m_vulkanObjs;
            ITexturesPtr m_textures;

            uint32_t m_currentFrameIndex{0};
            std::vector<FrameState> m_frames;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMES_H
