/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_IOPENXR_H
#define ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_IOPENXR_H

#include <Accela/Render/Eye.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vulkan/vulkan.h>

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace Accela::Render
{
    struct OXRSystemRequirements
    {
        uint64_t minVulkanVersionSupported{0};
        uint64_t maxVulkanVersionSupported{0};
        std::vector<std::string> requiredInstanceExtensions;
        std::vector<std::string> requiredDeviceExtensions;
    };

    struct OXRViewConfigurationView
    {
        uint32_t recommendedSwapChainSampleCount{0};
        uint32_t recommendedImageWidth{0};
        uint32_t recommendedImageHeight{0};
    };

    struct OXREyeView
    {
        // Pose
        glm::vec3 posePosition{0.0f};
        glm::quat poseOrientation{};

        // Fov
        float leftTanHalfAngle{0.0f};
        float rightTanHalfAngle{0.0f};
        float upTanHalfAngle{0.0f};
        float downTanHalfAngle{0.0f};
    };

    class IOpenXR
    {
        public:

            using Ptr = std::shared_ptr<IOpenXR>;

        public:

            virtual ~IOpenXR() = default;

            //
            // Lifecycle methods - Should be called in this order
            //
            [[nodiscard]] virtual bool CreateInstance() = 0;
            [[nodiscard]] virtual bool FetchSystem() = 0;
            [[nodiscard]] virtual std::expected<VkPhysicalDevice, bool> GetOpenXRPhysicalDevice(VkInstance vkInstance) const = 0;
            [[nodiscard]] virtual bool OnVulkanInitialized(VkInstance vkInstance,
                                                           VkPhysicalDevice vkPhysicalDevice,
                                                           VkDevice vkDevice,
                                                           uint32_t vkGraphicsQueueFamilyIndex) = 0;
            [[nodiscard]] virtual bool CreateSession() = 0;
            virtual void Destroy() = 0;

            //
            // Accessors
            //
            [[nodiscard]] virtual std::optional<OXRSystemRequirements> GetSystemRequirements() const = 0;
            [[nodiscard]] virtual std::vector<OXRViewConfigurationView> GetSystemEyeConfigurationViews() const = 0;

            [[nodiscard]] virtual bool IsSystemAvailable() const = 0;
            [[nodiscard]] virtual bool IsSessionCreated() const = 0;

            //
            // Frame methods - Should be called in this order
            //
            virtual void ProcessEvents() = 0;
            virtual void BeginFrame() = 0;
            virtual void AcquireSwapChainImages() = 0;
            virtual void RefreshViewData() = 0;
            [[nodiscard]] virtual std::vector<OXREyeView> GetFrameEyeViews() const = 0;
            [[nodiscard]] virtual VkImage GetFrameEyeImage(const Eye& eye) const = 0;
            virtual void ReleaseSwapChainImages() = 0;
            virtual void EndFrame() = 0;
    };
}

#endif //ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_IOPENXR_H
