/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINEDESKTOP_SRC_OPENXR_H
#define ACCELAENGINE_ACCELAENGINEDESKTOP_SRC_OPENXR_H

#include <Accela/Render/IOpenXR.h>

#include <Accela/Common/Log/ILogger.h>

#include <openxr/openxr.hpp>
#include <openxr/openxr_platform.h>

#include <optional>
#include <vector>
#include <string>
#include <expected>

namespace Accela::Render
{
    class OpenXR : public IOpenXR
    {
        public:

            OpenXR(Common::ILogger::Ptr logger, std::string appName, uint32_t appVersion);

            [[nodiscard]] bool CreateInstance() override;
            [[nodiscard]] bool FetchSystem() override;
            [[nodiscard]] std::expected<VkPhysicalDevice, bool> GetOpenXRPhysicalDevice(VkInstance vkInstance) const override;
            [[nodiscard]] bool OnVulkanInitialized(VkInstance vkInstance,
                                                   VkPhysicalDevice vkPhysicalDevice,
                                                   VkDevice vkDevice,
                                                   uint32_t vkGraphicsQueueFamilyIndex) override;
            [[nodiscard]] bool CreateSession() override;
            void Destroy() override;

            [[nodiscard]] bool IsSystemAvailable() const override;
            [[nodiscard]] bool IsSessionCreated() const override;
            [[nodiscard]] std::optional<OXRSystemRequirements> GetSystemRequirements() const override;
            [[nodiscard]] std::vector<OXRViewConfigurationView> GetSystemEyeConfigurationViews() const override;

            void ProcessEvents() override;

            void BeginFrame() override;
            void AcquireSwapChainImages() override;
            void RefreshViewData() override;
            [[nodiscard]] std::vector<OXREyeView> GetFrameEyeViews() const override;
            [[nodiscard]] VkImage GetFrameEyeImage(const Eye& eye) const override;
            void ReleaseSwapChainImages() override;
            void EndFrame() override;

        private:

            enum class State
            {
                None,                   // Nothing initialized
                InstanceCreated,        // An XrInstance has been created
                SystemFound,            // An XrSystem has been retrieved
                VulkanProvided,         // Vulkan render objects have been provided
                SessionCreated          // An XrSession is created
            };

            struct System
            {
                XrSystemId xrSystemId{0};
                XrSystemProperties xrSystemProperties{};
                OXRSystemRequirements systemRequirements{};
                XrViewConfigurationType xrViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
                std::vector<OXRViewConfigurationView> oxrViewConfigurationViews;
            };

            struct SwapChainInfo
            {
                XrSwapchain swapChain{XR_NULL_HANDLE};
                int64_t swapChainFormat{0};
                std::vector<XrSwapchainImageVulkanKHR> xrSwapChainImages;
            };

            struct Frame
            {
                Frame()
                {
                    xrFrameState.type = XR_TYPE_FRAME_STATE;
                }

                // Populated in BeginFrame()
                XrFrameState xrFrameState{};

                // Populated in AcquireSwapChainImages()
                std::vector<VkImage> viewImages;

                // Populated in RefreshViewData()
                std::vector<XrPosef> viewPoses;
                std::vector<XrFovf> viewFovs;
            };

        private:

            void DestroyPostInstance();
            void DestroyInstance();

            [[nodiscard]] bool CreateXrInstance();
            void DestroyXrInstance();

            [[nodiscard]] bool CreateXrDebugMessenger();
            void DestroyXrDebugMessenger();

            void DestroyXrSystemInfo();

            [[nodiscard]] bool CreateXrSession();
            void DestroyXrSession();

            [[nodiscard]] bool CreateXrSwapChains();
            void DestroyXrSwapChains();

            [[nodiscard]] bool CreateXrReferenceSpace();
            void DestroyXrReferenceSpace();

        private:

            Common::ILogger::Ptr m_logger;
            std::string m_appName;
            uint32_t m_appVersion;

            State m_state{State::None};

            // Instance
            // Valid when state >= InstanceCreated
            XrInstance m_xrInstance{XR_NULL_HANDLE};
            std::vector<std::string> m_enabledInstanceExtensions;
            PFN_xrGetVulkanGraphicsRequirementsKHR m_xrGetVulkanGraphicsRequirementsKHR{nullptr};
            PFN_xrGetVulkanInstanceExtensionsKHR m_xrGetVulkanInstanceExtensionsKHR{nullptr};
            PFN_xrGetVulkanDeviceExtensionsKHR m_xrGetVulkanDeviceExtensionsKHR{nullptr};
            PFN_xrGetVulkanGraphicsDeviceKHR m_xrGetVulkanGraphicsDeviceKHR{nullptr};
            PFN_xrDestroyDebugUtilsMessengerEXT m_xrDestroyDebugUtilsMessengerEXT{nullptr};
            PFN_xrCreateDebugUtilsMessengerEXT m_xrCreateDebugUtilsMessengerEXT{nullptr};
            XrDebugUtilsMessengerEXT m_xrDebugMessenger{XR_NULL_HANDLE};

            // System
            // Valid when state >= SystemFound
            std::optional<System> m_system;

            // Vulkan Objects
            // Valid when state >= VulkanProvided
            VkInstance m_vkInstance{VK_NULL_HANDLE};
            VkPhysicalDevice m_vkPhysicalDevice{VK_NULL_HANDLE};
            VkDevice m_vkDevice{VK_NULL_HANDLE};
            uint32_t m_vkGraphicsQueueFamilyIndex{0};

            // Session
            // Valid when state >= SessionIdle
            XrSession m_xrSession{XR_NULL_HANDLE};
            XrSessionState m_xrSessionState{XrSessionState::XR_SESSION_STATE_UNKNOWN};
            std::vector<SwapChainInfo> m_colorSwapChainInfos;
            XrSpace m_localSpace = XR_NULL_HANDLE;

            // Frame
            // Valid during a BeginFrame()..EndFrame() scope
            Frame m_frame{};
    };
}

#endif //ACCELAENGINE_ACCELAENGINEDESKTOP_SRC_OPENXR_H
