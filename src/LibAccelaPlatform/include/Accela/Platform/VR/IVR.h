/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_IVR_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_IVR_H

#include "VRDevice.h"

#include "../Eye.h"

#include <memory>
#include <vector>

namespace Accela::Platform
{
    enum class EyeTextureType
    {
        Vulkan
    };

    enum class EyeTextureColorSpace
    {
        Auto,
        Gamma,
        Linear
    };

    struct EyeTexture
    {
        void* pTextureData;
        EyeTextureType textureType;
        EyeTextureColorSpace textureColorSpace;
    };

    struct EyeTextureBounds
    {
        float uMin{0.0f};
        float vMin{1.0f};
        float uMax{0.0f};
        float vMax{1.0f};
    };

    enum class EyeTextureSubmitFlags
    {
        Default,
        Submit_VulkanTextureWithArrayData
    };

    struct EyeProjectionRaw
    {
        float leftTanHalfAngle{0.0f};
        float rightTanHalfAngle{0.0f};
        float topTanHalfAngle{0.0f};
        float bottomTanHalfAngle{0.0f};
    };

    /**
     * Interface for working with VR devices
     */
    class IVR
    {
        public:

            using Ptr = std::shared_ptr<IVR>;

        public:

            virtual ~IVR() = default;

            [[nodiscard]] virtual bool IsVRAvailable() const = 0;

            [[nodiscard]] virtual bool Startup() = 0;
            virtual void Shutdown() = 0;

            [[nodiscard]] virtual bool IsVRRunning() const = 0;
            virtual void WaitGetPoses() = 0;

            [[nodiscard]] virtual std::vector<VRDevice> GetDeviceStates() const = 0;
            [[nodiscard]] virtual glm::mat4 GetEyeToHeadTransform(const Eye& eye) const = 0;
            [[nodiscard]] virtual glm::mat4 GetEyeProjectionTransform(const Eye& eye, const float& nearClip, const float& farClip) const = 0;
            [[nodiscard]] virtual EyeProjectionRaw GetEyeProjectionRaw(const Eye& eye) const = 0;

            virtual void SubmitEyeTexture(
                const Platform::Eye& eye,
                const EyeTexture& texture,
                const EyeTextureBounds& textureBounds,
                const EyeTextureSubmitFlags& textureSubmitFlags) const = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_IVR_H
