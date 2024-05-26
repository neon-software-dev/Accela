#include <Accela/Platform/VR/OpenVR.h>

namespace Accela::Platform
{

static inline glm::mat4 hmd_mat4(const vr::HmdMatrix44_t & m) noexcept {
    return {
        m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0],
        m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1],
        m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2],
        m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]
    };
}

static inline glm::mat4 hmd_mat3x4(const vr::HmdMatrix34_t & m) noexcept {
    return {
        m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
        m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
        m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
        m.m[0][3], m.m[1][3], m.m[2][3], 1.0f
    };
}


OpenVR::OpenVR(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

bool OpenVR::IsVRAvailable() const
{
    return vr::VR_IsRuntimeInstalled() && vr::VR_IsHmdPresent();
}

bool OpenVR::Startup()
{
    m_logger->Log(Common::LogLevel::Info, "OpenVR: Initializing VR");

    if (!IsVRAvailable())
    {
        m_logger->Log(Common::LogLevel::Info, "OpenVR: VR is not available");
        return true;
    }

    vr::EVRInitError initError{vr::VRInitError_None};
    m_pVRSystem = vr::VR_Init(&initError, vr::EVRApplicationType::VRApplication_Scene, nullptr);

    if (initError != vr::VRInitError_None)
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "OpenVR: VR_Init call failure, error code: {}", std::string{VR_GetVRInitErrorAsSymbol(initError)});
        return false;
    }

    return true;
}

void OpenVR::Shutdown()
{
    if (!IsVRRunning()) { return; }

    m_logger->Log(Common::LogLevel::Info, "OpenVR: Shutting down VR");

    vr::VR_Shutdown();
    m_pVRSystem = nullptr;
}

bool OpenVR::IsVRRunning() const
{
    return m_pVRSystem != nullptr;
}

void OpenVR::WaitGetPoses()
{
    if (!IsVRRunning()) { return; }

    m_deviceStates.clear();

    auto trackedDevicePoses = std::vector<vr::TrackedDevicePose_t>(vr::k_unMaxTrackedDeviceCount);
    const vr::EVRCompositorError error = vr::VRCompositor()->WaitGetPoses(trackedDevicePoses.data(), vr::k_unMaxTrackedDeviceCount, nullptr, 0);

    if (error != vr::VRCompositorError_None)
    {
        m_logger->Log(Common::LogLevel::Error, "OpenVR: WaitGetPoses error: {}", (unsigned int)error);
        return;
    }

    for (unsigned int deviceIndex = 0; deviceIndex < vr::k_unMaxTrackedDeviceCount; ++deviceIndex)
    {
        if (trackedDevicePoses[deviceIndex].bPoseIsValid)
        {
            const auto deviceClass = m_pVRSystem->GetTrackedDeviceClass(deviceIndex);

            VRDevice device{};

            switch (deviceClass)
            {
                case vr::TrackedDeviceClass_HMD:
                    device.type = VRDevice::Type::Headset;
                    break;
                default:
                    continue;
            }

            device.poseTransform = hmd_mat3x4(trackedDevicePoses[deviceIndex].mDeviceToAbsoluteTracking);

            m_deviceStates.push_back(device);
        }
    }
}

std::vector<VRDevice> OpenVR::GetDeviceStates() const
{
    return m_deviceStates;
}

glm::mat4 OpenVR::GetEyeToHeadTransform(const Eye& eye) const
{
    vr::Hmd_Eye vrEye = vr::Hmd_Eye::Eye_Left;
    if (eye == Eye::Right) { vrEye = vr::Hmd_Eye::Eye_Right; }

    return hmd_mat3x4(m_pVRSystem->GetEyeToHeadTransform(vrEye));
}

glm::mat4 OpenVR::GetEyeProjectionTransform(const Eye& eye, const float& nearClip, const float& farClip) const
{
    vr::Hmd_Eye vrEye = vr::Hmd_Eye::Eye_Left;
    if (eye == Eye::Right) { vrEye = vr::Hmd_Eye::Eye_Right; }

    return hmd_mat4(m_pVRSystem->GetProjectionMatrix(vrEye, nearClip, farClip));
}

EyeProjectionRaw OpenVR::GetEyeProjectionRaw(const Eye& eye) const
{
    vr::Hmd_Eye vrEye = vr::Hmd_Eye::Eye_Left;
    if (eye == Eye::Right) { vrEye = vr::Hmd_Eye::Eye_Right; }

    EyeProjectionRaw projectionRaw{};

    m_pVRSystem->GetProjectionRaw(
        vrEye,
        &projectionRaw.leftTanHalfAngle,
        &projectionRaw.rightTanHalfAngle,
        &projectionRaw.topTanHalfAngle,
        &projectionRaw.bottomTanHalfAngle
    );

    // NOTE!! OpenVR returns swapped bottom/top angles for some reason, we swap them back here
    // TODO: Verify whether this is the case with other VR displays
    // https://github.com/ValveSoftware/openvr/issues/816
    std::swap(projectionRaw.topTanHalfAngle, projectionRaw.bottomTanHalfAngle);

    return projectionRaw;
}

void OpenVR::SubmitEyeTexture(const Eye& eye,
                              const EyeTexture& texture,
                              const EyeTextureBounds& textureBounds,
                              const EyeTextureSubmitFlags& textureSubmitFlags) const
{
    vr::EVREye vrEye{};
    switch (eye)
    {
        case Eye::Left: vrEye = vr::EVREye::Eye_Left; break;
        case Eye::Right: vrEye = vr::EVREye::Eye_Right; break;
    }

    vr::ETextureType vrTextureType{};
    switch (texture.textureType)
    {
        case EyeTextureType::Vulkan: vrTextureType = vr::ETextureType::TextureType_Vulkan; break;
    }

    vr::EColorSpace vrColorSpace{};
    switch (texture.textureColorSpace)
    {
        case EyeTextureColorSpace::Auto: vrColorSpace = vr::EColorSpace::ColorSpace_Auto; break;
        case EyeTextureColorSpace::Gamma: vrColorSpace = vr::EColorSpace::ColorSpace_Gamma; break;
        case EyeTextureColorSpace::Linear: vrColorSpace = vr::EColorSpace::ColorSpace_Linear; break;
    }

    const vr::Texture_t vrTexture = {texture.pTextureData, vrTextureType, vrColorSpace};

    vr::VRTextureBounds_t vrTextureBounds{};
    vrTextureBounds.uMin = textureBounds.uMin;
    vrTextureBounds.uMax = textureBounds.uMax;
    vrTextureBounds.vMin = textureBounds.vMin;
    vrTextureBounds.vMax = textureBounds.vMax;

    vr::EVRSubmitFlags vrSubmitFlags{};
    switch (textureSubmitFlags)
    {
        case EyeTextureSubmitFlags::Default:
            vrSubmitFlags = vr::EVRSubmitFlags::Submit_Default;
        break;
        case EyeTextureSubmitFlags::Submit_VulkanTextureWithArrayData:
            vrSubmitFlags = vr::EVRSubmitFlags::Submit_VulkanTextureWithArrayData;
        break;
    }

    vr::VRCompositor()->Submit(vrEye, &vrTexture, &vrTextureBounds, vrSubmitFlags);
}

}
