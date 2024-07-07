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
VulkanFramebufferPtr VulkanObjs::GetSwapChainFrameBuffer(const uint32_t& imageIndex) const noexcept { return m_swapChainFrameBuffers[imageIndex]; }
VulkanRenderPassPtr VulkanObjs::GetGPassRenderPass() const noexcept { return m_gPassRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetScreenRenderPass() const noexcept { return m_screenRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetSwapChainBlitRenderPass() const noexcept { return m_swapChainBlitRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetShadow2DRenderPass() const noexcept { return m_shadow2DRenderPass; }
VulkanRenderPassPtr VulkanObjs::GetShadowCubeRenderPass() const noexcept { return m_shadowCubeRenderPass; }

bool VulkanObjs::Initialize(bool enableValidationLayers, const RenderSettings& renderSettings)
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

    if (!CreateSwapChain())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to initialize Vulkan swap chain");
        return false;
    }

    if (!CreateSwapChainBlitRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create swap chain blit render pass");
        return false;
    }

    if (!CreateSwapChainFrameBuffers())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create swap chain framebuffers");
        return false;
    }

    if (!CreateGPassRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create gpass render pass");
        return false;
    }

    if (!CreateScreenRenderPass())
    {
        m_logger->Log(Common::LogLevel::Error, "VulkanObjs: Failed to create screen render pass");
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
    DestroyScreenRenderPass();
    DestroyGPassRenderPass();
    DestroySwapChainFrameBuffers();
    DestroySwapChainBlitRenderPass();
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

bool VulkanObjs::CreateSwapChain()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating Vulkan swap chain");

    const auto previousSwapChain = m_swapChain;
    if (previousSwapChain != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "CreateSwapChain: Re-using previous swap chain");
    }

    auto swapChain = std::make_shared<VulkanSwapChain>(m_logger, m_vulkanCalls, m_vma, m_physicalDevice, m_device);

    if (!swapChain->Create(m_surface, previousSwapChain, m_renderSettings->presentMode))
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

bool VulkanObjs::CreateGPassRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating gpass render pass");

    const auto numGPassLayers = m_renderSettings->presentToHeadset ? 2 : 1;

    //
    // Framebuffer attachments
    //
    VulkanRenderPass::Attachment colorAttachment(VulkanRenderPass::AttachmentType::Color);
    colorAttachment.description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    colorAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    const ImageAccess colorAttachmentAccess(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // Deferred Lighting SubPass writing to the color attachment
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        // Forward Lighting Objects SubPass writing to the color attachment
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        Layers(0, numGPassLayers),
        Levels(0, 1),
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VulkanRenderPass::Attachment positionAttachment(VulkanRenderPass::AttachmentType::Color);
    positionAttachment.description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    positionAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    positionAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    positionAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    positionAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    positionAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    positionAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    positionAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess positionAttachmentAccess(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        // Deferred Lighting Objects SubPass writing to the position attachment
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        // Deferred Lighting SubPass reading from the position attachment
        BarrierPoint(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
        Layers(0, numGPassLayers),
        Levels(0, 1),
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VulkanRenderPass::Attachment normalAttachment(VulkanRenderPass::AttachmentType::Color);
    normalAttachment.description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    normalAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    normalAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess normalAttachmentAccess = positionAttachmentAccess;

    VulkanRenderPass::Attachment objectDetailAttachment(VulkanRenderPass::AttachmentType::Color);
    objectDetailAttachment.description.format = VK_FORMAT_R32G32_UINT;
    objectDetailAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    objectDetailAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    objectDetailAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    objectDetailAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    objectDetailAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    objectDetailAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    objectDetailAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess materialAttachmentAccess = normalAttachmentAccess;

    VulkanRenderPass::Attachment ambientAttachment(VulkanRenderPass::AttachmentType::Color);
    ambientAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    ambientAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    ambientAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ambientAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ambientAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ambientAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ambientAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ambientAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess ambientAttachmentAccess = materialAttachmentAccess;

    VulkanRenderPass::Attachment diffuseAttachment(VulkanRenderPass::AttachmentType::Color);
    diffuseAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    diffuseAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    diffuseAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    diffuseAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    diffuseAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    diffuseAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    diffuseAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    diffuseAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess diffuseAttachmentAccess = ambientAttachmentAccess;

    VulkanRenderPass::Attachment specularAttachment(VulkanRenderPass::AttachmentType::Color);
    specularAttachment.description.format = VK_FORMAT_R8G8B8A8_SRGB;
    specularAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    specularAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    specularAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    specularAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    specularAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    specularAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    specularAttachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const ImageAccess specularAttachmentAccess = diffuseAttachmentAccess;

    VulkanRenderPass::Attachment depthAttachment(VulkanRenderPass::AttachmentType::Depth);
    depthAttachment.description.format = m_physicalDevice->GetDepthBufferFormat();
    depthAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const ImageAccess depthAttachmentAccess(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        // Deferred Lighting Objects SubPass using the depth attachment
        BarrierPoint(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        // Forward Lighting SubPass using the depth attachment
        BarrierPoint(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        Layers(0, numGPassLayers),
        Levels(0, 1),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    //
    // Deferred Lighting Objects Subpass
    //
    VulkanRenderPass::Subpass deferredLightingObjectsSubpass{};

    deferredLightingObjectsSubpass.colorAttachmentRefs = {
        {.attachment = Offscreen_Attachment_Color,    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_Position, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_Normal,   .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_ObjectDetail, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_Ambient,  .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_Diffuse,  .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_Specular, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };

    deferredLightingObjectsSubpass.depthAttachmentRef =
        {.attachment = Offscreen_Attachment_Depth, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    //
    // Deferred Lighting Subpass
    //
    VulkanRenderPass::Subpass deferredLightingSubpass{};

    deferredLightingSubpass.colorAttachmentRefs = {
        {.attachment = Offscreen_Attachment_Color,    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };

    deferredLightingSubpass.inputAttachmentRefs = {
        {.attachment = Offscreen_Attachment_Position, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {.attachment = Offscreen_Attachment_Normal,   .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {.attachment = Offscreen_Attachment_ObjectDetail, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {.attachment = Offscreen_Attachment_Ambient,  .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {.attachment = Offscreen_Attachment_Diffuse,  .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {.attachment = Offscreen_Attachment_Specular, .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };

    //
    // Forward Lighting Objects Subpass
    //
    VulkanRenderPass::Subpass forwardLightingObjectsSubpass{};

    forwardLightingObjectsSubpass.colorAttachmentRefs = {
        {.attachment = Offscreen_Attachment_Color,    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.attachment = Offscreen_Attachment_ObjectDetail, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };

    forwardLightingObjectsSubpass.depthAttachmentRef =
        {.attachment = Offscreen_Attachment_Depth,    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    //
    // SubPass Dependencies
    //

    // Deferred Lighting Objects must have finished writing object data before Deferred Lighting Render can read it
    VkSubpassDependency dep0{};
    dep0.srcSubpass = GPassRenderPass_SubPass_DeferredLightingObjects;
    dep0.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep0.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep0.dstSubpass = GPassRenderPass_SubPass_DeferredLightingRender;
    dep0.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep0.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dep0.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Deferred Lighting Objects must have finished writing depth data before Forward Lighting can use it
    VkSubpassDependency dep1{};
    dep1.srcSubpass = GPassRenderPass_SubPass_DeferredLightingObjects;
    dep1.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep1.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dep1.dstSubpass = GPassRenderPass_SubPass_ForwardLightingObjects;
    dep1.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep1.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dep1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Deferred Lighting Render must have finished writing color data before Forward Lighting Objects can write to it
    VkSubpassDependency dep2{};
    dep2.srcSubpass = GPassRenderPass_SubPass_DeferredLightingRender;
    dep2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep2.dstSubpass = GPassRenderPass_SubPass_ForwardLightingObjects;
    dep2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

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
    m_gPassRenderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!m_gPassRenderPass->Create(
        {
            colorAttachment,
            positionAttachment,
            normalAttachment,
            objectDetailAttachment,
            ambientAttachment,
            diffuseAttachment,
            specularAttachment,
            depthAttachment
        },
        {
            colorAttachmentAccess,
            positionAttachmentAccess,
            normalAttachmentAccess,
            materialAttachmentAccess,
            ambientAttachmentAccess,
            diffuseAttachmentAccess,
            specularAttachmentAccess,
            depthAttachmentAccess
        },
        {
            deferredLightingObjectsSubpass,
            deferredLightingSubpass,
            forwardLightingObjectsSubpass
        },
        { dep0, dep1, dep2 },
        viewMasks,
        correlationMask,
        "Offscreen"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateOffscreenRenderPass: Failed to create the offscreen render pass");
        return false;
    }

    return true;
}

void VulkanObjs::DestroyGPassRenderPass()
{
    if (m_gPassRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying gpass render pass");
        m_gPassRenderPass->Destroy();
        m_gPassRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateScreenRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating screen render pass");

    //
    // Framebuffer attachments
    //
    VulkanRenderPass::Attachment colorAttachment(VulkanRenderPass::AttachmentType::Color);
    colorAttachment.description.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    const ImageAccess colorAttachmentAccess(
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        Layers(0, 1),
        Levels(0, 1),
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VulkanRenderPass::Attachment depthAttachment(VulkanRenderPass::AttachmentType::Depth);
    depthAttachment.description.format = m_physicalDevice->GetDepthBufferFormat();
    depthAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const ImageAccess depthAttachmentAccess(
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        BarrierPoint(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        BarrierPoint(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        Layers(0, 1),
        Levels(0, 1),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    //
    // Screen Subpass
    //
    VulkanRenderPass::Subpass screenSubpass{};

    screenSubpass.colorAttachmentRefs = {
        {.attachment = Screen_Attachment_Color,    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };

    screenSubpass.depthAttachmentRef =
        {.attachment = Screen_Attachment_Depth,    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    //
    // Create the render pass
    //
    m_screenRenderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!m_screenRenderPass->Create(
        {
            colorAttachment,
            depthAttachment
        },
        {
            colorAttachmentAccess,
            depthAttachmentAccess
        },
        {
            screenSubpass
        },
        { },
        std::nullopt,
        std::nullopt,
        "Screen"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateScreenRenderPass: Failed to create the screen render pass");
        return false;
    }

    return true;
}

void VulkanObjs::DestroyScreenRenderPass()
{
    if (m_screenRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying screen render pass");
        m_screenRenderPass->Destroy();
        m_screenRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateSwapChainBlitRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating swap chain blit render pass");

    VulkanRenderPass::Attachment colorAttachment(VulkanRenderPass::AttachmentType::Color);
    colorAttachment.description.format = m_swapChain->GetConfig()->surfaceFormat.format;
    colorAttachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    const ImageAccess colorAttachmentAccess(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        BarrierPoint(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
        Layers(0, 1),
        Levels(0, 1),
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VulkanRenderPass::Subpass swapChainBlitPass{};

    swapChainBlitPass.colorAttachmentRefs = {
        {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };

    m_swapChainBlitRenderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!m_swapChainBlitRenderPass->Create(
        {colorAttachment},
        {colorAttachmentAccess},
        {swapChainBlitPass},
        {},
        std::nullopt,
        std::nullopt,
        "SwapChainBlit"))
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateBlitRenderPass: Failed to create the swap chain blit render pass");
        return false;
    }

    return true;
}

void VulkanObjs::DestroySwapChainBlitRenderPass()
{
    if (m_swapChainBlitRenderPass != nullptr)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Destroying swap chain blit render pass");
        m_swapChainBlitRenderPass->Destroy();
        m_swapChainBlitRenderPass = nullptr;
    }
}

bool VulkanObjs::CreateShadow2DRenderPass()
{
    m_logger->Log(Common::LogLevel::Info, "VulkanObjs: Creating shadow 2d render pass");

    m_shadow2DRenderPass = CreateShadowRenderPass(std::nullopt, std::nullopt, 1, "Shadow");

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

    m_shadowCubeRenderPass = CreateShadowRenderPass(viewMasks, correlationMask, 6, "ShadowCube");

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
                                                       unsigned int depthNumLayers,
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

    const ImageAccess depthAttachmentAccess(
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        BarrierPoint(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        BarrierPoint(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        Layers(0, depthNumLayers),
        Levels(0, 1),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    VulkanRenderPass::Subpass shadowPass{};

    VkAttachmentReference vkDepthAttachmentReference{};
    vkDepthAttachmentReference.attachment = 0;
    vkDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowPass.depthAttachmentRef = vkDepthAttachmentReference;

    // Manual external dependency to synchronize usage of the depth buffer that was written. Prevents having to record
    // ImageAccess operations for every shadow map texture that the main rendering flow uses.
    VkSubpassDependency dependency_readShadowDepthOutput{};
    dependency_readShadowDepthOutput.srcSubpass = 0;
    dependency_readShadowDepthOutput.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency_readShadowDepthOutput.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readShadowDepthOutput.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readShadowDepthOutput.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency_readShadowDepthOutput.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency_readShadowDepthOutput.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    auto renderPass = std::make_shared<VulkanRenderPass>(m_logger, m_vulkanCalls, m_physicalDevice, m_device);
    if (!renderPass->Create(
        {depthAttachment},
        {depthAttachmentAccess},
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
            m_swapChainBlitRenderPass,
            {vkImageView},
            swapChainExtent,
            1,
            std::format("SwapChain-RenderTexture{}", imageIndex)))
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
    // Create a new swap chain for the surface. (Note that it internally destroys the old one as needed)
    //
    if (!CreateSwapChain())
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceInvalidated: Failed to create swap chain");
        return false;
    }

    //
    // Destroy and then re-create the swap chain blit render, as the format/details of the swap chain might have changed
    //
    DestroySwapChainBlitRenderPass();

    if (!CreateSwapChainBlitRenderPass())
    {
        m_logger->Log(Common::LogLevel::Info, "OnSurfaceInvalidated: Failed to create swap chain blit render pass");
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
