/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKANOBJS_H
#define LIBACCELARENDERERVK_SRC_VULKANOBJS_H

#include "ForwardDeclares.h"

#include "Vulkan/VulkanCommon.h"

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <string>
#include <utility>
#include <optional>
#include <vector>

namespace Accela::Render
{
    /**
     * Manges the static Vulkan objects that are created once per run,
     * which most Vulkan subsystems need access to.
     */
    class VulkanObjs
    {
        public:

            VulkanObjs(std::string appName,
                       uint32_t appVersion,
                       Common::ILogger::Ptr logger,
                       IVulkanCallsPtr vulkanCalls,
                       IVulkanContextPtr vulkanContext);

            bool Initialize(bool enableValidationLayers,
                            const RenderSettings& renderSettings);
            void Destroy();

            bool OnSurfaceInvalidated();
            bool OnSurfaceLost();
            bool OnRenderSettingsChanged(const RenderSettings& renderSettings);

            void WaitForDeviceIdle();

            [[nodiscard]] RenderSettings GetRenderSettings() const noexcept;
            [[nodiscard]] IVulkanCallsPtr GetCalls() const noexcept;
            [[nodiscard]] IVulkanContextPtr GetContext() const noexcept;
            [[nodiscard]] VulkanInstancePtr GetInstance() const noexcept;
            [[nodiscard]] VulkanSurfacePtr GetSurface() const noexcept;
            [[nodiscard]] VulkanPhysicalDevicePtr GetPhysicalDevice() const noexcept;
            [[nodiscard]] VulkanDevicePtr GetDevice() const noexcept;
            [[nodiscard]] IVMAPtr GetVMA() const noexcept;
            [[nodiscard]] VulkanCommandPoolPtr GetTransferCommandPool() const noexcept;
            [[nodiscard]] VulkanSwapChainPtr GetSwapChain() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetSwapChainRenderPass() const noexcept;
            [[nodiscard]] VulkanFramebufferPtr GetSwapChainFrameBuffer(const uint32_t& imageIndex) const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetOffscreenRenderPass() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetShadowRenderPass() const noexcept;

        private:

            bool CreateInstance(const std::string& appName, uint32_t appVersion, bool enableValidationLayers);
            void DestroyInstance() noexcept;

            bool CreateSurface();
            void DestroySurface() noexcept;

            bool CreatePhysicalDevice();
            void DestroyPhysicalDevice() noexcept;

            bool CreateLogicalDevice();
            void DestroyLogicalDevice() noexcept;

            bool InitVMA();
            void DestroyVMA();

            bool CreateSwapChain(PresentMode presentMode);
            void DestroySwapChain();

            bool CreateSwapChainRenderPass();
            void DestroySwapChainRenderPass();
            bool CreateOffscreenRenderPass();
            void DestroyOffscreenRenderPass();
            bool CreateShadowRenderPass();
            void DestroyShadowRenderPass();

            bool CreateSwapChainFrameBuffers();
            void DestroySwapChainFrameBuffers();

        private:

            std::string m_appName;
            uint32_t m_appVersion;
            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vulkanCalls;
            IVulkanContextPtr m_vulkanContext;

            std::optional<RenderSettings> m_renderSettings;

            VulkanInstancePtr m_instance;
            VulkanSurfacePtr m_surface;
            VulkanPhysicalDevicePtr m_physicalDevice;
            VulkanDevicePtr m_device;
            IVMAPtr m_vma;

            VulkanCommandPoolPtr m_transferCommandPool;

            VulkanSwapChainPtr m_swapChain;
            VulkanRenderPassPtr m_swapChainRenderPass;
            VulkanRenderPassPtr m_shadowRenderPass;
            std::vector<VulkanFramebufferPtr> m_swapChainFrameBuffers;

            VulkanRenderPassPtr m_offscreenRenderPass;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKANOBJS_H
