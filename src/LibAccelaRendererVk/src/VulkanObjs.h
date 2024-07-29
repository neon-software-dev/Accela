/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
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
            [[nodiscard]] VulkanFramebufferPtr GetSwapChainFrameBuffer(const uint32_t& imageIndex) const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetGPassRenderPass() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetScreenRenderPass() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetSwapChainBlitRenderPass() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetShadowCascadedRenderPass() const noexcept;
            [[nodiscard]] VulkanRenderPassPtr GetShadowCubeRenderPass() const noexcept;

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

            bool CreateSwapChain();
            void DestroySwapChain();

            bool CreateGPassRenderPass();
            void DestroyGPassRenderPass();
            bool CreateScreenRenderPass();
            void DestroyScreenRenderPass();
            bool CreateSwapChainBlitRenderPass();
            void DestroySwapChainBlitRenderPass();
            bool CreateShadowCascadedRenderPass();
            void DestroyShadowCascadedRenderPass();
            bool CreateShadowCubeRenderPass();
            void DestroyShadowCubeRenderPass();

            bool CreateSwapChainFrameBuffers();
            void DestroySwapChainFrameBuffers();

        private:

            VulkanRenderPassPtr CreateShadowRenderPass(const std::optional<std::vector<uint32_t>>& multiViewMasks,
                                                       const std::optional<uint32_t>& multiViewCorrelationMask,
                                                       unsigned int depthNumLayers,
                                                       const std::string& tag);

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
            std::vector<VulkanFramebufferPtr> m_swapChainFrameBuffers;

            VulkanRenderPassPtr m_gPassRenderPass; // Renders the world into the gpass framebuffer
            VulkanRenderPassPtr m_screenRenderPass; // Renders screen sprites into the screen framebuffer
            VulkanRenderPassPtr m_swapChainBlitRenderPass; // Combines the gpass and screen output into the swap chain framebuffer
            VulkanRenderPassPtr m_shadowCascadedRenderPass; // Renders a cascaded directional shadow pass into a light framebuffer
            VulkanRenderPassPtr m_shadowCubeRenderPass; // Renders a cubic point shadow pass into a light framebuffer
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKANOBJS_H
