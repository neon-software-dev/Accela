/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanObjs.h"

#include "Renderer/RendererCommon.h"

#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanSurface.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanFramebuffer.h"
#include "Vulkan/VulkanCommandPool.h"

#include "VMA/VMA.h"
#include "VMA/VMAUtil.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>

#include <format>
#include <algorithm>

namespace Accela::Render
{

VulkanObjs::VulkanObjs(std::string appName,
                       uint32_t appVersion,
                       Common::ILogger::Ptr logger,
                       IVulkanCallsPtr vulkanCalls,
                       IVulkanContextPtr vulkanContext)
   : m_appName(std::move(appName))
   , m_appVersion(appVersion)
   , m_logger(std::move(logger))
   , m_vulkanCalls(std::move(vulkanCalls))
   , m_vulkanContext(std::move(vulkanContext))
{

}

RenderSettings VulkanObjs::GetRenderSettings() const noexcept { return *m_renderSettings; }
IVulkanCallsPtr VulkanObjs::GetCalls() const noexcept { return m_vulkanCalls; }
IVulkanContextPtr VulkanObjs::GetContext() const noexcept { return m_vulkanContext; }
VulkanInstancePtr VulkanObjs::GetInstance() const noexcept { return m_instance; }
VulkanSurfacePtr VulkanObjs::GetSurface() const noexcept { return m_surface; }
VulkanPhysicalDevicePtr VulkanObjs::GetPhysicalDevice() const noexcept { return m_physicalDevice; }
VulkanDevicePtr VulkanObjs::GetDevice() const noexcept { return m_device; }
IVMAPtr VulkanObjs::GetVMA() const noexcept { return m_vma; }
VulkanCommandPoolPtr VulkanObjs::GetTransferCommandPool() const noexcept { return m_transferCommandPool; }
VulkanSwapChainPtr VulkanObjs::GetSwapChain() const noexcept { return m_swapChain; }
VulkanRenderPassPtr VulkanObjs::GetSwapChainRenderPass() const noexcept { return m_swapChainRenderPass; }
VulkanFramebufferPtr VulkanObjs::GetSwapChainFrameBuffer(const uint32_t& imageIndex) const noexcept { return m_swapChainFrameBuffers[imageIndex]; }
VulkanRenderPassPtr VulkanObjs::GetOffscreenRenderPass() const noexcept { return m_offscreenRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetShadow2DRenderPass() const noexcept { return m_shadow2DRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetShadowCubeRenderPass() const noexcept { return m_shadowCubeRenderPass; }

bool VulkanObjs::Initialize(bool enableValidationLayers,
                            const RenderSettings& renderSettings)
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Initializing Vulkan objects");

    m_renderSettings = renderSettings;

    if (!m_vulkanCalls->InitGlobalCalls())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to initialize global Vulkan calls");
        return false;
    }

    if (!CreateInstance(m_appName, m_appVersion, enableValidationLayers))
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create Vulkan instance");
        return false;
    }

    if (!CreateSurface())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create Vulkan surface");
        return false;
    }

    if (!CreatePhysicalDevice())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create Vulkan physical device");
        return false;
    }

    if (!CreateLogicalDevice())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create Vulkan logical device");
        return false;
    }

    if (!InitVMA())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to initialize VMA system");
        return false;
    }

    if (!CreateSwapChain(renderSettings.presentMode))
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to initialize Vulkan swap chain");
        return false;
    }

    if (!CreateSwapChainRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create swap chain render pass");
        return false;
    }

    if (!CreateSwapChainFrameBuffers())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create swap chain framebuffers");
        return false;
    }

    if (!CreateOffscreenRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create offscreen render pass");
        return false;
    }

    if (!CreateShadow2DRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create shadow 2d render pass");
        return false;
    }

    if (!CreateShadowCubeRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create shadow cube render pass");
        return false;
    }

    return true;
}

void VulkanObjs::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan objects");

    DestroyShadowCubeRenderPass();
    DestroyShadow2DRenderPass();
    DestroyOffscreenRenderPass();
    DestroySwapChainFrameBuffers();
    DestroySwapChainRenderPass();
    DestroySwapChain();
    DestroyVMA();
    DestroyLogicalDevice();
    DestroyPhysicalDevice();
    DestroySurface();
    DestroyInstance();

    m_renderSettings = std::nullopt;
}

bool VulkanObjs::CreateInstance(const std::string& appName, uint32_t appVersion, bool enableValidationLayers)
{
    if (m_instance != nullptr)
    {
        m_logger->Log(Common::LogLevel::Warning, "VulkanObjs: Instance already exists, ignoring");
        return true;
    }

    m_logger->Log(Common::LogLevel::Info, "CreateInstance: Creating a Vulkan instance");

    const auto vulkanInstance = std::make_shared<VulkanInstance>(m_logger, m_vulkanCalls, m_vulkanContext);
    if (!vulkanInstance->CreateInstance(appName, appVersion, enableValidationLayers))
    {
        m_logger->Log(Common::LogLevel::Error, "CreateInstance: Failed to create a Vulkan instance");
        return false;
    }

    m_instance = vulkanInstance;

    return true;
}

void VulkanObjs::DestroyInstance() noexcept
{
    if (m_instance != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan instance");
        m_instance->Destroy();
        m_instance = nullptr;
    }
}

bool VulkanObjs::CreateSurface()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating a Vulkan surface");

    const auto vulkanSurface = std::make_shared<VulkanSurface>(m_logger, m_vulkanCalls, m_vulkanContext);
    if (!vulkanSurface->Create(m_instance))
    {
        return false;
    }

    m_surface = vulkanSurface;

    return true;
}

void VulkanObjs::DestroySurface() noexcept
{
    if (m_surface != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan surface");
        m_surface->Destroy();
        m_surface = nullptr;
    }
}

bool VulkanObjs::CreatePhysicalDevice()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Choosing a Vulkan physical device");

    // Get the list of physical devices that support Vulkan
    const auto physicalDevices = VulkanPhysicalDevice::EnumerateAll(
        m_logger,
        m_vulkanCalls,
        m_vulkanContext,
        m_instance
    );

    // Prune out unsuitable physical devices
    std::vector<VulkanPhysicalDevicePtr> suitablePhysicalDevices;

    std::ranges::copy_if(physicalDevices, std::back_inserter(suitablePhysicalDevices), [this](const auto& physicalDevice){
        m_logger->Log(Common::LogLevel::Info, "CreatePhysicalDevice: Discovered physical device: {}", physicalDevice->GetDeviceName());
        return physicalDevice->IsDeviceSuitable(m_surface);
    });

    if (suitablePhysicalDevices.empty())
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreatePhysicalDevice: No suitable devices found");
        return false;
    }

    // Sort remaining physical devices by rating
    std::ranges::sort(suitablePhysicalDevices, [](const auto& lhs, const auto& rhs){
        return lhs->GetDeviceRating() < rhs->GetDeviceRating();
    });

    m_physicalDevice = suitablePhysicalDevices.back();

    m_logger->Log(Common::LogLevel::Info, "CreatePhysicalDevice: Chose physical device: {}", m_physicalDevice->GetDeviceName());

    return true;
}

void VulkanObjs::DestroyPhysicalDevice() noexcept
{
    if (m_physicalDevice != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan physical device");
        m_physicalDevice = nullptr;
    }
}

bool VulkanObjs::CreateLogicalDevice()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating a Vulkan logical device");

    //
    // Create the device
    //
    const auto device = std::make_shared<VulkanDevice>(m_logger, m_vulkanCalls, m_vulkanContext);
    if (!device->Create(m_physicalDevice, m_surface))
    {
        return false;
    }

    m_device = device;

    //
    // Create a transfer command pool for the device
    //
    const auto transferCommandPool = std::make_shared<VulkanCommandPool>(m_logger, m_vulkanCalls, m_device);
    if (!transferCommandPool->Create(
        m_physicalDevice->GetGraphicsQueueFamilyIndex().value(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        "Transfer"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateLogicalDevice: Failed to create transfer command pool");
        m_device->Destroy();
        m_device = nullptr;
        return false;
    }

    m_transferCommandPool = transferCommandPool;

    return true;
}

void VulkanObjs::DestroyLogicalDevice() noexcept
{
    if (m_transferCommandPool != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan logical device transfer command pool");
        m_transferCommandPool->Destroy();
        m_transferCommandPool = nullptr;
    }

    if (m_device != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan logical device");
        WaitForDeviceIdle();
        m_device->Destroy();
        m_device = nullptr;
    }
}

bool VulkanObjs::InitVMA()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Initializing VMA system");

    const auto vmaFuncs = m_vulkanCalls->GetVMAFuncs();
    const auto vmaVulkanFunctions = ToVmaVulkanFunctions(vmaFuncs);

    VmaAllocatorCreateInfo allocatorCreateInfo{};
    allocatorCreateInfo.vulkanApiVersion = VULKAN_API_VERSION;
    allocatorCreateInfo.instance = m_instance->GetVkInstance();
    allocatorCreateInfo.physicalDevice = m_physicalDevice->GetVkPhysicalDevice();
    allocatorCreateInfo.device = m_device->GetVkDevice();
    allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;
    allocatorCreateInfo.flags = 0;

    auto vmaOpt = VMA::CreateInstance(m_logger, allocatorCreateInfo);
    if (!vmaOpt.has_value())
    {
        return false;
    }

    m_vma = *vmaOpt;

    return true;
}

void VulkanObjs::DestroyVMA()
{
    if (m_vma != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying VMA system");

        m_vma->DestroyInstance();
        m_vma = nullptr;
    }
}

bool VulkanObjs::CreateSwapChain(PresentMode presentMode)
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating Vulkan swap chain");

    const auto previousSwapChain = m_swapChain;
    if (previousSwapChain != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "CreateSwapChain: Re-using previous swap chain");
    }

    auto swapChain = std::make_shared<VulkanSwapChain>(m_logger, m_vulkanCalls, m_vma, m_physicalDevice, m_device);

    if (!swapChain->Create(m_surface, previousSwapChain, presentMode))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateSwapChain: Failed to create swap chain");
        return false;
    }

    // Destroy previous swap chain (if any) before switching over to the new one
    DestroySwapChain();

    // Switch over to the new swap chain
    m_swapChain = swapChain;

    return true;
}

void VulkanObjs::DestroySwapChain()
{
    if (m_swapChain != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying Vulkan swap chain");
        m_swapChain->Destroy();
        m_swapChain = nullptr;
    }
}

bool VulkanObjs::CreateSwapChainRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating swap chain render pass");

    VulkanRenderPass::Attachment colorAttachment(VulkanRenderPass::AttachmentType::Color);
    colorAttachment.description.format = m_swapChain->GetConfig()->surfaceFormat.format;
    colorAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VulkanRenderPass::Subpass blitPass{};

    VkAttachmentReference vkColorAttachmentReference{};
    vkColorAttachmentReference.attachment = 0;
    vkColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    blitPass.colorAttachmentRefs.push_back(vkColorAttachmentReference);

    m_swapChainRenderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!m_swapChainRenderPass->Create({colorAttachment}, {blitPass}, {}, std::nullopt, std::nullopt, "SwapChain"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateSwapChainRenderPass: Failed to create the swap chain render pass");
        return false;
    }

    return true;
}

void VulkanObjs::DestroySwapChainRenderPass()
{
    if (m_swapChainRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying swap chain render pass");
        m_swapChainRenderPass->Destroy();
        m_swapChainRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateOffscreenRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating offscreen render pass");

    //
    // Framebuffer attachments
    //

    VulkanRenderPass::Attachment colorAttachment(VulkanRenderPass::AttachmentType::Color);
    colorAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    colorAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment positionAttachment(VulkanRenderPass::AttachmentType::Color);
    positionAttachment.description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    positionAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    positionAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    positionAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    positionAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    positionAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    positionAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    positionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment normalAttachment(VulkanRenderPass::AttachmentType::Color);
    normalAttachment.description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    normalAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    normalAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment materialAttachment(VulkanRenderPass::AttachmentType::Color);
    materialAttachment.description.format = VK_FORMAT_R32_UINT;
    materialAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    materialAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    materialAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    materialAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    materialAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    materialAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    materialAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment ambientAttachment(VulkanRenderPass::AttachmentType::Color);
    ambientAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    ambientAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    ambientAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ambientAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ambientAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ambientAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ambientAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ambientAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment diffuseAttachment(VulkanRenderPass::AttachmentType::Color);
    diffuseAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    diffuseAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    diffuseAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    diffuseAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    diffuseAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    diffuseAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    diffuseAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    diffuseAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment specularAttachment(VulkanRenderPass::AttachmentType::Color);
    specularAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    specularAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    specularAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    specularAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    specularAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    specularAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    specularAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    specularAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Attachment depthAttachment(VulkanRenderPass::AttachmentType::Depth);
    depthAttachment.description.format = m_physicalDevice->GetDepthBufferFormat();
    depthAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //
    // GPass Subpass
    //
    VkAttachmentReference vkGpassColorAttachmentReference{};
    vkGpassColorAttachmentReference.attachment = 0;
    vkGpassColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassPositionAttachmentReference{};
    vkGpassPositionAttachmentReference.attachment = 1;
    vkGpassPositionAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassNormalAttachmentReference{};
    vkGpassNormalAttachmentReference.attachment = 2;
    vkGpassNormalAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassMaterialAttachmentReference{};
    vkGpassMaterialAttachmentReference.attachment = 3;
    vkGpassMaterialAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassAmbientAttachmentReference{};
    vkGpassAmbientAttachmentReference.attachment = 4;
    vkGpassAmbientAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassDiffuseAttachmentReference{};
    vkGpassDiffuseAttachmentReference.attachment = 5;
    vkGpassDiffuseAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassSpecularAttachmentReference{};
    vkGpassSpecularAttachmentReference.attachment = 6;
    vkGpassSpecularAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkGpassDepthAttachmentReference{};
    vkGpassDepthAttachmentReference.attachment = 7;
    vkGpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VulkanRenderPass::Subpass gSubpass{};
    gSubpass.colorAttachmentRefs.push_back(vkGpassColorAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassPositionAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassNormalAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassMaterialAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassAmbientAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassDiffuseAttachmentReference);
    gSubpass.colorAttachmentRefs.push_back(vkGpassSpecularAttachmentReference);
    gSubpass.depthAttachmentRef = vkGpassDepthAttachmentReference;

    //
    // Deferred Lighting Subpass
    //

    VkAttachmentReference vkLightingColorAttachmentReference{};
    vkLightingColorAttachmentReference.attachment = 0;
    vkLightingColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkLightingPositionAttachmentReference{};
    vkLightingPositionAttachmentReference.attachment = 1;
    vkLightingPositionAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference vkLightingNormalAttachmentReference{};
    vkLightingNormalAttachmentReference.attachment = 2;
    vkLightingNormalAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference vkLightingMaterialAttachmentReference{};
    vkLightingMaterialAttachmentReference.attachment = 3;
    vkLightingMaterialAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference vkLightingAmbientAttachmentReference{};
    vkLightingAmbientAttachmentReference.attachment = 4;
    vkLightingAmbientAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference vkLightingDiffuseAttachmentReference{};
    vkLightingDiffuseAttachmentReference.attachment = 5;
    vkLightingDiffuseAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference vkLightingSpecularAttachmentReference{};
    vkLightingSpecularAttachmentReference.attachment = 6;
    vkLightingSpecularAttachmentReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Subpass lightingSubpass{};
    lightingSubpass.colorAttachmentRefs.push_back(vkLightingColorAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingPositionAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingNormalAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingMaterialAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingAmbientAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingDiffuseAttachmentReference);
    lightingSubpass.inputAttachmentRefs.push_back(vkLightingSpecularAttachmentReference);

    //
    // Forward Subpass
    //

    VkAttachmentReference vkForwardColorAttachmentReference{};
    vkForwardColorAttachmentReference.attachment = 0;
    vkForwardColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference vkForwardDepthAttachmentReference{};
    vkForwardDepthAttachmentReference.attachment = 7;
    vkForwardDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VulkanRenderPass::Subpass forwardSubpass{};
    forwardSubpass.colorAttachmentRefs.push_back(vkForwardColorAttachmentReference);
    forwardSubpass.depthAttachmentRef = vkForwardDepthAttachmentReference;

    //
    // Subpass Dependencies
    //

    // Color dependency between GPass and Lighting subpasses
    VkSubpassDependency dependency_readGpassColorOutput{};
    dependency_readGpassColorOutput.srcSubpass = Offscreen_GPassSubpass_Index;
    dependency_readGpassColorOutput.dstSubpass = Offscreen_LightingSubpass_Index;
    dependency_readGpassColorOutput.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_readGpassColorOutput.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    dependency_readGpassColorOutput.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency_readGpassColorOutput.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency_readGpassColorOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Depth dependency between GPass and Forward subpasses
    VkSubpassDependency dependency_readGpassDepthOutput{};
    dependency_readGpassDepthOutput.srcSubpass = Offscreen_GPassSubpass_Index;
    dependency_readGpassDepthOutput.dstSubpass = Offscreen_ForwardSubpass_Index;
    dependency_readGpassDepthOutput.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readGpassDepthOutput.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readGpassDepthOutput.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readGpassDepthOutput.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readGpassDepthOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Color dependency between Lighting and Forward subpasses
    VkSubpassDependency dependency_readLightingColorOutput{};
    dependency_readLightingColorOutput.srcSubpass = Offscreen_LightingSubpass_Index;
    dependency_readLightingColorOutput.dstSubpass = Offscreen_ForwardSubpass_Index;
    dependency_readLightingColorOutput.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_readLightingColorOutput.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency_readLightingColorOutput.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency_readLightingColorOutput.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency_readLightingColorOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Color dependency between Forward subpass and external (swap chain) render pass
    VkSubpassDependency dependency_readColorOutput{};
    dependency_readColorOutput.srcSubpass = Offscreen_ForwardSubpass_Index;
    dependency_readColorOutput.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency_readColorOutput.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_readColorOutput.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency_readColorOutput.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency_readColorOutput.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency_readColorOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //
    // Multiview settings
    //
    std::optional<std::vector<uint32_t>> viewMasks;
    std::optional<uint32_t> correlationMask;

    // If we're presenting to a headset the render pass should use multiview to render each eye in each draw call
    if (m_renderSettings->presentToHeadset)
    {
        // If in VR mode, the offscreen subpasses are multiviewed for rendering twice, once for each eye
        viewMasks = {0b00000011, 0b00000011, 0b00000011};
        correlationMask = 0b00000011;
    }

    //
    // Create the render pass
    //
    m_offscreenRenderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!m_offscreenRenderPass->Create(
        {
            colorAttachment,
            positionAttachment,
            normalAttachment,
            materialAttachment,
            ambientAttachment,
            diffuseAttachment,
            specularAttachment,
            depthAttachment
        },
        {
            gSubpass,
            lightingSubpass,
            forwardSubpass
        },
        {
            dependency_readGpassColorOutput,
            dependency_readGpassDepthOutput,
            dependency_readLightingColorOutput,
            dependency_readColorOutput
        },
        viewMasks,
        correlationMask,
        "Offscreen"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateOffscreenRenderPass: Failed to create the offscreen render pass");
        return false;
    }

    return true;
}

void VulkanObjs::DestroyOffscreenRenderPass()
{
    if (m_offscreenRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying offscreen render pass");
        m_offscreenRenderPass->Destroy();
        m_offscreenRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateShadow2DRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating shadow 2d render pass");

    m_shadow2DRenderPass = CreateShadowRenderPass(std::nullopt, std::nullopt, "Shadow");

    return m_shadow2DRenderPass != nullptr;
}

void VulkanObjs::DestroyShadow2DRenderPass()
{
    if (m_shadow2DRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying shadow 2d render pass");
        m_shadow2DRenderPass->Destroy();
        m_shadow2DRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateShadowCubeRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating shadow cube render pass");

    const std::vector<uint32_t> viewMasks = {0b00111111};
    const uint32_t correlationMask = 0b00111111;

    m_shadowCubeRenderPass = CreateShadowRenderPass(viewMasks, correlationMask, "ShadowCube");

    return m_shadowCubeRenderPass != nullptr;
}

void VulkanObjs::DestroyShadowCubeRenderPass()
{
    if (m_shadowCubeRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying shadow cube render pass");
        m_shadowCubeRenderPass->Destroy();
        m_shadowCubeRenderPass = nullptr;
    }
}

VulkanRenderPassPtr VulkanObjs::CreateShadowRenderPass(const std::optional<std::vector<uint32_t>>& multiViewMasks,
                                                       const std::optional<uint32_t>& multiViewCorrelationMask,
                                                       const std::string& tag)
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating {} render pass", tag);

    VulkanRenderPass::Attachment depthAttachment(VulkanRenderPass::AttachmentType::Depth);
    depthAttachment.description.format = VK_FORMAT_D32_SFLOAT; // TODO PERF: need this many bytes?
    depthAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VulkanRenderPass::Subpass shadowPass{};

    VkAttachmentReference vkDepthAttachmentReference{};
    vkDepthAttachmentReference.attachment = 0;
    vkDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowPass.depthAttachmentRef = vkDepthAttachmentReference;

    // Dependency for following render passes to use the depth buffer that was written
    VkSubpassDependency dependency_readShadowDepthOutput{};
    dependency_readShadowDepthOutput.srcSubpass = 0;
    dependency_readShadowDepthOutput.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency_readShadowDepthOutput.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readShadowDepthOutput.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readShadowDepthOutput.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readShadowDepthOutput.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readShadowDepthOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    auto renderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!renderPass->Create(
        {depthAttachment},
        {shadowPass},
        {dependency_readShadowDepthOutput},
        multiViewMasks,
        multiViewCorrelationMask,
        tag))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateShadowRenderPass: Failed to create {} render pass", tag);
        return nullptr;
    }

    return renderPass;
}

bool VulkanObjs::CreateSwapChainFrameBuffers()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating swap chain frame buffers");

    if (!m_swapChainFrameBuffers.empty())
    {
        m_logger->Log(Common::LogLevel::Error, "CreateSwapChainFrameBuffers: framebuffers already existed");
        return false;
    }

    const auto swapChainVkExtent = m_swapChain->GetConfig()->extent;
    const auto swapChainExtent = Size(swapChainVkExtent.width, swapChainVkExtent.height);

    uint32_t imageIndex = 0;

    for (const auto& vkImageView : m_swapChain->GetSwapChainImageViews())
    {
        auto framebuffer = std::make_shared<VulkanFramebuffer>(m_logger, m_vulkanCalls, m_device);
        if (!framebuffer->Create(
            m_swapChainRenderPass,
            {vkImageView},
            swapChainExtent,
            1,
            std::format("SwapChain-Image{}", imageIndex)))
        {
            m_logger->Log(Common::LogLevel::Error, "CreateSwapChainFrameBuffers: Failed to create a swap chain framebuffer");
            return false;
        }

        m_swapChainFrameBuffers.push_back(framebuffer);

        imageIndex++;
    }

    return true;
}

void VulkanObjs::DestroySwapChainFrameBuffers()
{
    if (!m_swapChainFrameBuffers.empty())
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying swap chain framebuffers");

        for (auto& framebuffer: m_swapChainFrameBuffers)
        {
            framebuffer->Destroy();
        }

        m_swapChainFrameBuffers.clear();
    }
}

bool VulkanObjs::OnSurfaceInvalidated()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Handling invalidated surface");

    WaitForDeviceIdle();

    if (!m_renderSettings) { return false; }

    //
    // Create a new swap chain for the surface. (Note that it internally
    // destroys the old one as needed)
    //
    if (!CreateSwapChain(m_renderSettings->presentMode))
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceInvalidated: Failed to create swap chain");
        return false;
    }

    //
    // Destroy and then re-create the render pass for the swap chain, as the
    // format/details of the swap chain might have changed
    //
    DestroySwapChainRenderPass();

    if (!CreateSwapChainRenderPass())
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceInvalidated: Failed to create swap chain render pass");
        return false;
    }

    //
    // Destroy then re-create the swap chain framebuffers
    //
    DestroySwapChainFrameBuffers();

    if (!CreateSwapChainFrameBuffers())
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceInvalidated: Failed to create swap chain framebuffers");
        return false;
    }

    return true;
}

bool VulkanObjs::OnSurfaceLost()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Handling lost surface");

    WaitForDeviceIdle();

    //
    // Destroy the old surface and create a new one
    //
    DestroySurface();
    if (!CreateSurface())
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceLost: Failed to create surface");
        return false;
    }

    //
    // Then go through the surface invalidation flow
    //
    return OnSurfaceInvalidated();
}

bool VulkanObjs::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    m_renderSettings = renderSettings;

    return OnSurfaceInvalidated();
}

void VulkanObjs::WaitForDeviceIdle()
{
    if (m_device != nullptr)
    {
        m_vulkanCalls->vkDeviceWaitIdle(m_device->GetVkDevice());
    }
}

}
