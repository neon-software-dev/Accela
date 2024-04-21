/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORMDESKTOP_SRC_VR_OPENVR_H
#define LIBACCELAPLATFORMDESKTOP_SRC_VR_OPENVR_H

#include <Accela/Platform/VR/IVR.h>

#include <Accela/Common/Log/ILogger.h>

#include <openvr.h>

#include <vector>

namespace Accela::Platform
{
    class OpenVR : public IVR
    {
        public:

            explicit OpenVR(Common::ILogger::Ptr logger);

            [[nodiscard]] bool IsVRAvailable() const override;

            [[nodiscard]] bool Startup() override;
            void Shutdown() override;

            [[nodiscard]] bool IsVRRunning() const override;
            void WaitGetPoses() override;

            [[nodiscard]] std::vector<VRDevice> GetDeviceStates() const override;
            [[nodiscard]] glm::mat4 GetEyeToHeadTransform(const Eye& eye) const override;
            [[nodiscard]] glm::mat4 GetEyeProjectionTransform(const Eye& eye, const float& nearClip, const float& farClip) const override;
            [[nodiscard]] EyeProjectionRaw GetEyeProjectionRaw(const Eye& eye) const override;

            void SubmitEyeTexture(
                const Platform::Eye& eye,
                const EyeTexture& texture,
                const EyeTextureBounds& textureBounds,
                const EyeTextureSubmitFlags& textureSubmitFlags) const override;

        private:

            Common::ILogger::Ptr m_logger;

            vr::IVRSystem* m_pVRSystem{nullptr};

            std::vector<VRDevice> m_deviceStates;
    };
}

#endif //LIBACCELAPLATFORMDESKTOP_SRC_VR_OPENVR_H
