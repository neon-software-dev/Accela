/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/DesktopVulkanContext.h>

#include <openvr.h>

#include <vector>

namespace Accela::Engine
{

Platform::Eye ToPlatformEye(const Render::Eye& eye)
{
    Platform::Eye pEye{};

    switch (eye)
    {
        case Render::Eye::Left: pEye = Platform::Eye::Left; break;
        case Render::Eye::Right: pEye = Platform::Eye::Right; break;
    }

    return pEye;
}

DesktopVulkanContext::DesktopVulkanContext(Platform::IPlatform::Ptr platform)
    : m_platform(std::move(platform))
{

}

std::vector<std::string> ExtensionBytesToVec(const std::vector<char>& extensionsBytes)
{
    std::vector<std::string> extensions;

    std::string curExtStr;

    for (const char& c : extensionsBytes)
    {
        if (c == ' ')
        {
            extensions.push_back(curExtStr);
            curExtStr.clear();
        }
        else
        {
            curExtStr += c;
        }
    }

    if (!curExtStr.empty())
    {
        extensions.push_back(curExtStr);
    }

    return extensions;
}

bool DesktopVulkanContext::GetRequiredInstanceExtensions(std::set<std::string>& extensions) const
{
    //
    // Get the extensions that SDL reports are required for it to create a Vulkan surface
    //
    std::vector<std::string> windowExtensions;

    if (!m_platform->GetWindow()->GetVulkanRequiredExtensions(windowExtensions))
    {
        return false;
    }

    //
    // Get extensions needed for OpenVR to render, if applicable
    //
    std::vector<std::string> vrExtensions;

    if (m_platform->GetVR()->IsVRRunning())
    {
        uint32_t nBufferSize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(nullptr, 0);
        if (nBufferSize > 0)
        {
            std::vector<char> extensionsBytes(nBufferSize);
            vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(extensionsBytes.data(), nBufferSize);
            vrExtensions = ExtensionBytesToVec(extensionsBytes);
        }
    }

    //
    // Create list of all required extensions
    //
    for (const auto& extension : windowExtensions) { extensions.insert(extension); }
    for (const auto& extension : vrExtensions) { extensions.insert(extension); }

    return true;
}

bool DesktopVulkanContext::GetRequiredDeviceExtensions(VkPhysicalDevice vkPhysicalDevice, std::set<std::string>& extensions) const
{
    //
    // Get extensions needed to render to the current headset, if applicable
    //
    std::vector<std::string> vrExtensions;

    if (m_platform->GetVR()->IsVRRunning())
    {
        uint32_t nBufferSize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(vkPhysicalDevice, nullptr, 0);
        if (nBufferSize > 0)
        {
            std::vector<char> extensionsBytes(nBufferSize);
            vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(vkPhysicalDevice, extensionsBytes.data(), nBufferSize);
            vrExtensions = ExtensionBytesToVec(extensionsBytes);
        }
    }

    for (const auto& extension : vrExtensions) { extensions.insert(extension); }

    // Require swap chain extension for presenting to surface
    extensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    return true;
}

bool DesktopVulkanContext::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *pSurface) const
{
    return m_platform->GetWindow()->CreateVulkanSurface(instance, pSurface);
}

bool DesktopVulkanContext::GetSurfacePixelSize(std::pair<unsigned int, unsigned int>& size) const
{
    const auto sizeExpect = m_platform->GetWindow()->GetWindowSize();
    if (!sizeExpect)
    {
        return false;
    }

    size = *sizeExpect;

    return true;
}

bool DesktopVulkanContext::VR_InitOutput() const
{
    return m_platform->GetVR()->Startup();
}

void DesktopVulkanContext::VR_DestroyOutput() const
{
    m_platform->GetVR()->Shutdown();
}

void DesktopVulkanContext::VR_WaitGetPoses() const
{
    m_platform->GetVR()->WaitGetPoses();
}

std::optional<glm::mat4> DesktopVulkanContext::VR_GetHeadsetPose() const
{
    const auto devices = m_platform->GetVR()->GetDeviceStates();

    for (const auto& device : devices)
    {
        if (device.type == Platform::VRDevice::Type::Headset)
        {
            return device.poseTransform;
        }
    }

    return std::nullopt;
}

glm::mat4 DesktopVulkanContext::VR_GetEyeToHeadTransform(const Render::Eye& eye) const
{
    return m_platform->GetVR()->GetEyeToHeadTransform(ToPlatformEye(eye));
}

glm::mat4 DesktopVulkanContext::VR_GetEyeProjectionTransform(const Render::Eye& eye,
                                                             const float& nearClip,
                                                             const float& farClip) const
{
    return m_platform->GetVR()->GetEyeProjectionTransform(ToPlatformEye(eye), nearClip, farClip);
}

void DesktopVulkanContext::VR_GetEyeProjectionRaw(const Render::Eye& eye, float& leftTanHalfAngle, float& rightTanHalfAngle, float& topTanHalfAngle, float& bottomTanHalfAngle) const
{
    const auto eyeProjectionRaw = m_platform->GetVR()->GetEyeProjectionRaw(ToPlatformEye(eye));

    leftTanHalfAngle = eyeProjectionRaw.leftTanHalfAngle;
    rightTanHalfAngle = eyeProjectionRaw.rightTanHalfAngle;
    topTanHalfAngle = eyeProjectionRaw.topTanHalfAngle;
    bottomTanHalfAngle = eyeProjectionRaw.bottomTanHalfAngle;
}

void DesktopVulkanContext::VR_SubmitEyeRender(const Render::Eye& eye, const Render::HeadsetEyeRenderData& eyeRenderData) const
{
    auto* vulkanTextureData = new vr::VRVulkanTextureArrayData_t;
    vulkanTextureData->m_nImage = (uint64_t)eyeRenderData.vkImage;
    vulkanTextureData->m_pDevice = eyeRenderData.vkDevice;
    vulkanTextureData->m_pPhysicalDevice = eyeRenderData.vkPhysicalDevice;
    vulkanTextureData->m_pInstance = eyeRenderData.vkInstance;
    vulkanTextureData->m_pQueue = eyeRenderData.vkQueue;
    vulkanTextureData->m_nQueueFamilyIndex = eyeRenderData.queueFamilyIndex;
    vulkanTextureData->m_nWidth = eyeRenderData.width;
    vulkanTextureData->m_nHeight = eyeRenderData.height;
    vulkanTextureData->m_nFormat = eyeRenderData.format;
    vulkanTextureData->m_nSampleCount = eyeRenderData.sampleCount;
    vulkanTextureData->m_unArrayIndex = static_cast<unsigned int>(eye);
    vulkanTextureData->m_unArraySize = 2;

    Platform::EyeTexture platformEyeTexture{};
    platformEyeTexture.pTextureData = (void*)(vulkanTextureData);
    platformEyeTexture.textureType = Platform::EyeTextureType::Vulkan;
    platformEyeTexture.textureColorSpace = Platform::EyeTextureColorSpace::Auto;

    Platform::EyeTextureBounds platformEyeTextureBounds{};
    platformEyeTextureBounds.uMin = 0.0f;
    platformEyeTextureBounds.uMax = 1.0f;
    platformEyeTextureBounds.vMin = 0.0f;
    platformEyeTextureBounds.vMax = 1.0f;

    const Platform::EyeTextureSubmitFlags platformEyeTextureSubmitFlags = Platform::EyeTextureSubmitFlags::Submit_VulkanTextureWithArrayData;

    m_platform->GetVR()->SubmitEyeTexture(
        ToPlatformEye(eye),
        platformEyeTexture,
        platformEyeTextureBounds,
        platformEyeTextureSubmitFlags
    );
}

}
