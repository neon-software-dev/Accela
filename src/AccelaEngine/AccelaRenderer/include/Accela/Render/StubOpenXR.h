/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_STUBOPENXR_H
#define ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_STUBOPENXR_H

#include "IOpenXR.h"

namespace Accela::Render
{
    /**
     * Stub/no-op IOpenXR instance for clients that aren't interested in providing an
     * OpenXR implementation to use the Renderer.
     */
    class StubOpenXR : public IOpenXR
    {
        public:

            [[nodiscard]] bool CreateInstance() override { return false; }
            [[nodiscard]] bool FetchSystem() override { return false; }
            [[nodiscard]] std::expected<VkPhysicalDevice, bool> GetOpenXRPhysicalDevice(VkInstance) const override { return std::unexpected(false); }
            [[nodiscard]] bool OnVulkanInitialized(VkInstance, VkPhysicalDevice, VkDevice, uint32_t) override { return false; }
            [[nodiscard]] bool CreateSession() override { return false; }
            void Destroy() override { }
            [[nodiscard]] std::optional<OXRSystemRequirements> GetSystemRequirements() const override { return std::nullopt; }
            [[nodiscard]] std::vector<OXRViewConfigurationView> GetSystemEyeConfigurationViews() const override { return {}; }

            [[nodiscard]] bool IsSystemAvailable() const override { return false; }
            [[nodiscard]] bool IsSessionCreated() const override { return false; }
            void ProcessEvents() override { }
            void BeginFrame() override { }
            void AcquireSwapChainImages() override { }
            void RefreshViewData() override { }
            [[nodiscard]] std::vector<OXREyeView> GetFrameEyeViews() const override { return {}; }
            [[nodiscard]] VkImage GetFrameEyeImage(const Eye&) const override { return VK_NULL_HANDLE; }
            void ReleaseSwapChainImages() override { }
            void EndFrame() override { }
    };
}

#endif //ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_STUBOPENXR_H
