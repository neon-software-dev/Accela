/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "OpenXR.h"

#include <Accela/Render/Util/Vector.h>

#include <Accela/Common/Version.h>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <functional>
#include <sstream>

namespace Accela::Render
{

inline const char* GetXRErrorString(XrInstance xrInstance, XrResult result)
{
    static char string[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(xrInstance, result, string);
    return string;
}

template <typename T>
inline bool BitwiseCheck(const T &value, const T &checkValue) {
    return ((value & checkValue) == checkValue);
}

inline glm::quat FromOpenRX(const XrQuaternionf& quat)
{
    return {quat.w, quat.x, quat.y, quat.z};
}

inline glm::vec3 FromOpenRX(const XrVector3f& vec)
{
    return {vec.x, vec.y, vec.z};
}

static XrBool32 DebugMessengerCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                                       XrDebugUtilsMessageTypeFlagsEXT messageTypes,
                                       const XrDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                       void* pUserData)
{
    Common::LogLevel logLevel{Common::LogLevel::Error};

    if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
    {
        logLevel = Common::LogLevel::Error;
    }
    else if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
    {
        logLevel = Common::LogLevel::Warning;
    }
    else if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
    {
        logLevel = Common::LogLevel::Info;
    }
    else if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
    {
        logLevel = Common::LogLevel::Debug;
    }

    const auto GetMessageTypeString = [](XrDebugUtilsMessageTypeFlagsEXT messageType) -> std::string {
        bool separator = false;

        std::string msgFlags;
        if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)) {
            msgFlags += "GENERAL";
            separator = true;
        }
        if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)) {
            if (separator) { msgFlags += ","; }
            msgFlags += "VALIDATION";
            separator = true;
        }
        if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) {
            if (separator) { msgFlags += ","; }
            msgFlags += "PERFORMANCE";
        }
        return msgFlags;
    };

    const std::string functionName = (pCallbackData->functionName) ? pCallbackData->functionName : "";
    const std::string messageTypeStr = GetMessageTypeString(messageTypes);
    const std::string messageId = (pCallbackData->messageId) ? pCallbackData->messageId : "";
    const std::string message = (pCallbackData->message) ? pCallbackData->message : "";

    ((Common::ILogger*)pUserData)->Log(logLevel,
       "[OpenXRMessage] ({}): {} / {} - {}", messageTypeStr, functionName, messageId, message
    );

    return XrBool32();
}

OpenXR::OpenXR(Common::ILogger::Ptr logger, std::string appName, uint32_t appVersion)
    : m_logger(std::move(logger))
    , m_appName(std::move(appName))
    , m_appVersion(appVersion)
{

}

bool OpenXR::CreateInstance()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating XrInstance");

    if (m_state >= State::InstanceCreated)
    {
        m_logger->Log(Common::LogLevel::Warning, "OpenXR::CreateInstance: State is already >= InstanceCreated");
        return true;
    }

    if (!CreateXrInstance())
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateInstance: Failed to create instance");
        return false;
    }

    //
    // Create an OpenXR Debug Messenger, if available
    //
    const bool enableDebugMessenger = std::ranges::any_of(m_enabledInstanceExtensions, [](const auto& enabledExtension){
        return enabledExtension == XR_EXT_DEBUG_UTILS_EXTENSION_NAME;
    });

    if (enableDebugMessenger)
    {
        if (!CreateXrDebugMessenger())
        {
            m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateInstance: Failed to create debug messenger");
        }
    }

    m_logger->Log(Common::LogLevel::Info, "OpenXR: XrInstance created");

    m_state = State::InstanceCreated;

    return true;
}

bool OpenXR::FetchSystem()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Fetching XrSystem");

    if (m_state < State::InstanceCreated)
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::FetchSystem: State needs to be >= InstanceCreated");
        return false;
    }

    //
    // Fetch OpenXR headset system state
    //
    XrSystemGetInfo systemGI{};
    systemGI.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGI.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId xrSystemId{};
    auto result = xrGetSystem(m_xrInstance, &systemGI, &xrSystemId);
    if (XR_FAILED(result))
    {
        if (result == XR_ERROR_FORM_FACTOR_UNAVAILABLE)
        {
            m_logger->Log(Common::LogLevel::Error, "OpenXR::FetchSystem: VR headset is currently unavailable");
        }
        else
        {
            m_logger->Log(Common::LogLevel::Error,
              "OpenXR::FetchSystem: No or unsupported VR headset found, error: {}", GetXRErrorString(m_xrInstance, result));
        }

        return false;
    }

    //
    // At this point a system is actively connected, fetch its properties
    //
    XrSystemProperties xrSystemProperties{};
    xrSystemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
    result = xrGetSystemProperties(m_xrInstance, xrSystemId, &xrSystemProperties);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetSystemProperties failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    m_logger->Log(Common::LogLevel::Info, "OpenXR::FetchSystem: Found available headset system: {}", xrSystemProperties.systemName);

    //
    // Look up the system's graphics requirements
    //
    XrGraphicsRequirementsVulkanKHR graphicsRequirements{};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR;
    result = m_xrGetVulkanGraphicsRequirementsKHR(m_xrInstance, xrSystemId, &graphicsRequirements);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetVulkanGraphicsRequirementsKHR failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    //
    // Look up the system's supported view configurations
    //
    std::vector<XrViewConfigurationType> viewConfigurations;

    uint32_t viewConfigurationCount = 0;
    result = xrEnumerateViewConfigurations(m_xrInstance, xrSystemId, 0, &viewConfigurationCount, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrEnumerateViewConfigurations failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    viewConfigurations.resize(viewConfigurationCount);
    result = xrEnumerateViewConfigurations(m_xrInstance, xrSystemId, viewConfigurationCount, &viewConfigurationCount, viewConfigurations.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrEnumerateViewConfigurations failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    const XrViewConfigurationType viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    if (!std::ranges::any_of(viewConfigurations, [&](const auto& viewConfiguration){
        return viewConfiguration == viewConfigurationType;
    }))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: System doesn't support a primary stereo view configuration (not a headset?)");
        return false;
    }

    //
    // Look up the system's view configuration views
    //
    std::vector<XrViewConfigurationView> xrViewConfigurationViews;

    uint32_t viewConfigurationViewCount = 0;
    result = xrEnumerateViewConfigurationViews(m_xrInstance, xrSystemId, viewConfigurationType, 0, &viewConfigurationViewCount, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrEnumerateViewConfigurationViews failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    xrViewConfigurationViews.resize(viewConfigurationViewCount);
    for (auto& viewConfigurationView : xrViewConfigurationViews)
    {
        viewConfigurationView.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    }
    result = xrEnumerateViewConfigurationViews(m_xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewConfigurationViewCount, &viewConfigurationViewCount, xrViewConfigurationViews.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrEnumerateViewConfigurationViews failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    std::vector<OXRViewConfigurationView> viewConfigurationViews;

    for (const auto& xrViewConfigurationView : xrViewConfigurationViews)
    {
        OXRViewConfigurationView oxrViewConfigurationView{};
        oxrViewConfigurationView.recommendedSwapChainSampleCount = xrViewConfigurationView.recommendedSwapchainSampleCount;
        oxrViewConfigurationView.recommendedImageWidth = xrViewConfigurationView.recommendedImageRectWidth;
        oxrViewConfigurationView.recommendedImageHeight = xrViewConfigurationView.recommendedImageRectHeight;
        viewConfigurationViews.push_back(oxrViewConfigurationView);
    }

    //
    // Look up the system's required vulkan instance extensions
    //
    std::vector<std::string> requiredInstanceExtensions;

    uint32_t extensionNamesSize = 0;
    result = m_xrGetVulkanInstanceExtensionsKHR(m_xrInstance, xrSystemId, 0, &extensionNamesSize, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetVulkanInstanceExtensionsKHR failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    std::vector<char> extensionNames(extensionNamesSize);
    result = m_xrGetVulkanInstanceExtensionsKHR(m_xrInstance, xrSystemId, extensionNamesSize, &extensionNamesSize, extensionNames.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetVulkanInstanceExtensionsKHR failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    // Split space-separated extension list into a vector of strings
    {
        std::stringstream streamData(extensionNames.data());
        std::string extension;
        while (std::getline(streamData, extension, ' '))
        {
            requiredInstanceExtensions.push_back(extension);
        }
    }

    //
    // Look up the system's required vulkan device extensions
    //
    std::vector<std::string> requiredDeviceExtensions;

    extensionNamesSize = 0;
    result = m_xrGetVulkanDeviceExtensionsKHR(m_xrInstance, xrSystemId, 0, &extensionNamesSize, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetVulkanDeviceExtensionsKHR failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    extensionNames = std::vector<char>(extensionNamesSize);
    result = m_xrGetVulkanDeviceExtensionsKHR(m_xrInstance, xrSystemId, extensionNamesSize, &extensionNamesSize, extensionNames.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::FetchSystem: xrGetVulkanDeviceExtensionsKHR failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    // Split space-separated extension list into vector of strings
    {
        std::stringstream streamData(extensionNames.data());
        std::string extension;
        while (std::getline(streamData, extension, ' '))
        {
            requiredDeviceExtensions.push_back(extension);
        }
    }

    //
    // Update local state
    //
    OXRSystemRequirements systemRequirements{};
    systemRequirements.minVulkanVersionSupported = graphicsRequirements.minApiVersionSupported;
    systemRequirements.maxVulkanVersionSupported = graphicsRequirements.maxApiVersionSupported;
    systemRequirements.requiredInstanceExtensions = requiredInstanceExtensions;
    systemRequirements.requiredDeviceExtensions = requiredDeviceExtensions;

    m_system = System{};
    m_system->xrSystemId = xrSystemId;
    m_system->xrSystemProperties = xrSystemProperties;
    m_system->systemRequirements = systemRequirements;
    m_system->xrViewConfigurationType = viewConfigurationType;
    m_system->oxrViewConfigurationViews = viewConfigurationViews;

    m_logger->Log(Common::LogLevel::Info, "OpenXR: System found: {}", xrSystemProperties.systemName);

    m_state = State::SystemFound;

    return true;
}

std::expected<VkPhysicalDevice, bool> OpenXR::GetOpenXRPhysicalDevice(VkInstance vkInstance) const
{
    if (m_state < State::SystemFound)
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::GetOpenXRPhysicalDevice: State must be >= SystemFound");
        return std::unexpected(false);
    }

    VkPhysicalDevice vkPhysicalDevice{VK_NULL_HANDLE};

    auto result = m_xrGetVulkanGraphicsDeviceKHR(m_xrInstance, m_system->xrSystemId, vkInstance, &vkPhysicalDevice);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::GetOpenXRPhysicalDevice: Failed to fetch OpenXR physical device, error: {}", GetXRErrorString(m_xrInstance, result));
        return std::unexpected(false);
    }

    if (vkPhysicalDevice == VK_NULL_HANDLE)
    {
        return std::unexpected(false);
    }

    return vkPhysicalDevice;
}

bool OpenXR::OnVulkanInitialized(VkInstance vkInstance,
                                 VkPhysicalDevice vkPhysicalDevice,
                                 VkDevice vkDevice,
                                 uint32_t vkGraphicsQueueFamilyIndex)
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Told that Vulkan objects have been initialized");

    if (m_state < State::SystemFound)
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::OnVulkanInitialized: State needs to be >= SystemFound");
        return false;
    }

    m_vkInstance = vkInstance;
    m_vkPhysicalDevice = vkPhysicalDevice;
    m_vkDevice = vkDevice;
    m_vkGraphicsQueueFamilyIndex = vkGraphicsQueueFamilyIndex;

    m_state = State::VulkanProvided;

    return true;
}

bool OpenXR::CreateSession()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating an XrSession");

    if (!CreateXrSession())
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateSession: Failed to create an OpenXR session");
        return false;
    }

    if (!CreateXrSwapChains())
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateSession: Failed to create OpenXR swap chains");
        DestroyXrSession();
        return false;
    }

    if (!CreateXrReferenceSpace())
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateSession: Failed to create reference space");
        DestroyXrSwapChains();
        DestroyXrSession();
        return false;
    }

    m_logger->Log(Common::LogLevel::Info, "OpenXR: Created an XrSession");

    m_state = State::SessionCreated;

    return true;
}

void OpenXR::Destroy()
{
    if (m_state == State::None)
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying");

    DestroyPostInstance();
    DestroyInstance();

    m_frame = {};

    m_vkInstance = VK_NULL_HANDLE;
    m_vkPhysicalDevice = VK_NULL_HANDLE;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkGraphicsQueueFamilyIndex = 0;

    m_state = State::None;
}

void OpenXR::DestroyPostInstance()
{
    DestroyXrReferenceSpace();
    DestroyXrSwapChains();
    DestroyXrSession();
    DestroyXrSystemInfo();
}

void OpenXR::DestroyInstance()
{
    DestroyXrDebugMessenger();
    DestroyXrInstance();
}

bool OpenXR::CreateXrInstance()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating XrInstance");

    //
    // Query OpenXR for available API layers
    //
    uint32_t apiLayerCount = 0;
    std::vector<XrApiLayerProperties> availableApiLayerProperties;
    auto result = xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrEnumerateApiLayerProperties failed, error: {}", (int)result);
        return false;
    }

    availableApiLayerProperties.resize(apiLayerCount);
    for (auto& apiLayerProperty : availableApiLayerProperties)
    {
        apiLayerProperty.type = XR_TYPE_API_LAYER_PROPERTIES;
    }
    result = xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, availableApiLayerProperties.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrEnumerateApiLayerProperties failed, error: {}", (int)result);
        return false;
    }

    //
    // Verify that all required API layers are available
    //
    std::vector<std::string> requiredApiLayers = {};
    std::vector<std::string> enabledApiLayers;

    for (const auto& requiredApiLayer : requiredApiLayers)
    {
        const auto layerAvailable = std::ranges::any_of(availableApiLayerProperties, [&](const auto& availableApiLayerProperty){
            return strcmp(requiredApiLayer.c_str(), availableApiLayerProperty.layerName) == 0;
        });

        if (!layerAvailable)
        {
            m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Required API layer not found: {}", requiredApiLayer);
            return false;
        }

        enabledApiLayers.push_back(requiredApiLayer);
    }

    //
    // Query OpenXR for available instance extensions
    //
    uint32_t extensionCount = 0;
    std::vector<XrExtensionProperties> availableExtensionProperties;
    result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrEnumerateInstanceExtensionProperties failed, error: {}", (int)result);
        return false;
    }

    availableExtensionProperties.resize(extensionCount);
    for (auto& extensionProperty : availableExtensionProperties)
    {
        extensionProperty.type = XR_TYPE_EXTENSION_PROPERTIES;
    }
    result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensionProperties.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrEnumerateInstanceExtensionProperties failed, error: {}", (int)result);
        return false;
    }

    //
    // Verify which required and optional instance extensions are available
    //
    std::vector<std::string> requiredInstanceExtensions;
    requiredInstanceExtensions.emplace_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);

    std::vector<std::string> optionalInstanceExtensions;
    optionalInstanceExtensions.emplace_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

    for (const auto& requiredInstanceExtension : requiredInstanceExtensions)
    {
        const auto extensionAvailable = std::ranges::any_of(availableExtensionProperties, [&](const auto& availableExtensionProperty){
            return strcmp(requiredInstanceExtension.c_str(), availableExtensionProperty.extensionName) == 0;
        });

        if (!extensionAvailable)
        {
            m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Required instance extension not found: {}", requiredInstanceExtension);
            return false;
        }

        m_enabledInstanceExtensions.emplace_back(requiredInstanceExtension);
    }

    for (const auto& optionalInstanceExtension : optionalInstanceExtensions)
    {
        const auto extensionAvailable = std::ranges::any_of(availableExtensionProperties, [&](const auto& availableExtensionProperty){
            return strcmp(optionalInstanceExtension.c_str(), availableExtensionProperty.extensionName) == 0;
        });

        if (!extensionAvailable)
        {
            m_logger->Log(Common::LogLevel::Warning, "OpenXR::CreateXrInstance: Optional instance extension not found: {}", optionalInstanceExtension);
            continue;
        }

        m_enabledInstanceExtensions.emplace_back(optionalInstanceExtension);
    }

    //
    // Create OpenXR instance
    //
    std::vector<const char*> enabledApiLayerCStrs;
    std::transform(std::begin(enabledApiLayers), std::end(enabledApiLayers),
                   std::back_inserter(enabledApiLayerCStrs), std::mem_fn(&std::string::c_str));

    std::vector<const char*> enabledInstanceExtensionCStrs;
    std::transform(std::begin(m_enabledInstanceExtensions), std::end(m_enabledInstanceExtensions),
                   std::back_inserter(enabledInstanceExtensionCStrs), std::mem_fn(&std::string::c_str));

    XrApplicationInfo xrApplicationInfo{};
    strncpy(xrApplicationInfo.applicationName, m_appName.c_str(), XR_MAX_APPLICATION_NAME_SIZE - 1);
    xrApplicationInfo.applicationVersion = m_appVersion;
    strncpy(xrApplicationInfo.engineName, Common::ACCELA_ENGINE_NAME, XR_MAX_ENGINE_NAME_SIZE);
    xrApplicationInfo.engineVersion = Common::ACCELA_ENGINE_VERSION;
    xrApplicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo instanceCI{};
    instanceCI.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.createFlags = 0;
    instanceCI.applicationInfo = xrApplicationInfo;
    instanceCI.enabledApiLayerCount = static_cast<uint32_t>(enabledApiLayerCStrs.size());
    instanceCI.enabledApiLayerNames = enabledApiLayerCStrs.data();
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensionCStrs.size());
    instanceCI.enabledExtensionNames = enabledInstanceExtensionCStrs.data();
    result = xrCreateInstance(&instanceCI, &m_xrInstance);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrCreateInstance failed, error: {}", (int)result);
        return false;
    }

    //
    // Fetch Vulkan-specific OpenXR functions
    //
    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&m_xrGetVulkanGraphicsRequirementsKHR)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Failed to fetch xrGetVulkanGraphicsRequirementsKHR function");
        return false;
    }
    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction *)&m_xrGetVulkanInstanceExtensionsKHR)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Failed to fetch xrGetVulkanGraphicsRequirementsKHR function");
        return false;
    }
    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction *)&m_xrGetVulkanDeviceExtensionsKHR)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Failed to fetch xrGetVulkanGraphicsRequirementsKHR function");
        return false;
    }
    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction *)&m_xrGetVulkanGraphicsDeviceKHR)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrInstance: Failed to fetch xrGetVulkanGraphicsRequirementsKHR function");
        return false;
    }

    //
    // Fetch instance properties
    //
    XrInstanceProperties instanceProperties{};
    instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
    result = xrGetInstanceProperties(m_xrInstance, &instanceProperties);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateXrInstance: xrGetInstanceProperties failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    m_logger->Log(Common::LogLevel::Info, "OpenXR: Created instance using runtime: {}, version: {}.{}.{}",
        instanceProperties.runtimeName,
        XR_VERSION_MAJOR(instanceProperties.runtimeVersion),
        XR_VERSION_MINOR(instanceProperties.runtimeVersion),
        XR_VERSION_PATCH(instanceProperties.runtimeVersion)
    );

    return true;
}

void OpenXR::DestroyXrInstance()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying XrInstance");

    if (m_xrInstance != XR_NULL_HANDLE)
    {
        xrDestroyInstance(m_xrInstance);
        m_xrInstance = XR_NULL_HANDLE;
    }

    m_enabledInstanceExtensions.clear();

    m_xrGetVulkanGraphicsRequirementsKHR = nullptr;
    m_xrGetVulkanInstanceExtensionsKHR = nullptr;
    m_xrGetVulkanDeviceExtensionsKHR = nullptr;
    m_xrGetVulkanGraphicsDeviceKHR = nullptr;
}

void OpenXR::DestroyXrSystemInfo()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying XrSystem");
    m_system = std::nullopt;
}

bool OpenXR::CreateXrDebugMessenger()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating debug messenger");

    // Resolve debug messenger extension functions
    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)&m_xrDestroyDebugUtilsMessengerEXT)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateDebugMessenger: Failed to fetch xrDestroyDebugUtilsMessengerEXT function");
        return false;
    }

    if (XR_FAILED(xrGetInstanceProcAddr(m_xrInstance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)&m_xrCreateDebugUtilsMessengerEXT)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateDebugMessenger: Failed to fetch xrCreateDebugUtilsMessengerEXT function");
        return false;
    }

    // Create debug messenger
    XrDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
    debugUtilsMessengerCI.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCI.messageSeverities =
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCI.messageTypes =
        XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    debugUtilsMessengerCI.userCallback = (PFN_xrDebugUtilsMessengerCallbackEXT)DebugMessengerCallback;
    debugUtilsMessengerCI.userData = (void*)m_logger.get();

    if (XR_FAILED(m_xrCreateDebugUtilsMessengerEXT(m_xrInstance, &debugUtilsMessengerCI, &m_xrDebugMessenger)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateXrDebugMessenger: xrCreateDebugUtilsMessengerEXT call failed");
        return false;
    }

    return true;
}

void OpenXR::DestroyXrDebugMessenger()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying debug messenger");

    if (m_xrDebugMessenger != XR_NULL_HANDLE && m_xrDestroyDebugUtilsMessengerEXT)
    {
        m_xrDestroyDebugUtilsMessengerEXT(m_xrDebugMessenger);
    }

    m_xrDestroyDebugUtilsMessengerEXT = nullptr;
    m_xrCreateDebugUtilsMessengerEXT= nullptr;
    m_xrDebugMessenger = XR_NULL_HANDLE;
}

bool OpenXR::CreateXrSession()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating XrSession");

    if (m_state < State::VulkanProvided)
    {
        m_logger->Log(Common::LogLevel::Warning, "OpenXR::CreateXrSession: State must be >= VulkanProvided");
        return false;
    }

    XrGraphicsBindingVulkanKHR graphicsBinding{};
    graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
    graphicsBinding.instance = m_vkInstance;
    graphicsBinding.physicalDevice = m_vkPhysicalDevice;
    graphicsBinding.device = m_vkDevice;
    graphicsBinding.queueFamilyIndex = m_vkGraphicsQueueFamilyIndex;
    graphicsBinding.queueIndex = 0; // TODO: Get from device

    XrSessionCreateInfo sessionCI{};
    sessionCI.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCI.next = &graphicsBinding;
    sessionCI.createFlags = 0;
    sessionCI.systemId = m_system->xrSystemId;

    auto result = xrCreateSession(m_xrInstance, &sessionCI, &m_xrSession);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Warning,
          "OpenXR::CreateXrSession: xrCreateSession failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    return true;
}

void OpenXR::DestroyXrSession()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying XrSession");

    if (m_xrSession != XR_NULL_HANDLE)
    {
        xrDestroySession(m_xrSession);
        m_xrSession = XR_NULL_HANDLE;
    }

    m_xrSessionState = XrSessionState::XR_SESSION_STATE_UNKNOWN;
    m_colorSwapChainInfos.clear();
    m_localSpace = XR_NULL_HANDLE;
}

bool OpenXR::CreateXrSwapChains()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating XrSwapChains");

    //
    // Fetch supported swapchain formats
    //
    uint32_t formatCount = 0;
    auto result = xrEnumerateSwapchainFormats(m_xrSession, 0, &formatCount, nullptr);
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateSwapChains: xrEnumerateSwapchainFormats failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    std::vector<int64_t> formats(formatCount);
    result = xrEnumerateSwapchainFormats(m_xrSession, formatCount, &formatCount, formats.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateSwapChains: xrEnumerateSwapchainFormats failed, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    const auto requiredSwapChainFormat = VK_FORMAT_R8G8B8A8_SRGB;

    if (!std::ranges::any_of(formats, [&](const auto& availableFormat){
        return availableFormat == requiredSwapChainFormat;
    }))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::CreateSwapChains: Required swap chain image format is not supported");
        return false;
    }

    // Create a swap chain for each system eye/view
    m_colorSwapChainInfos.resize(m_system->oxrViewConfigurationViews.size());

    for (size_t x = 0; x < m_system->oxrViewConfigurationViews.size(); ++x)
    {
        SwapChainInfo& colorSwapChainInfo = m_colorSwapChainInfos[x];

        XrSwapchainCreateInfo swapChainCI{};
        swapChainCI.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapChainCI.createFlags = 0;
        swapChainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        swapChainCI.format = requiredSwapChainFormat;
        swapChainCI.sampleCount = m_system->oxrViewConfigurationViews[x].recommendedSwapChainSampleCount;
        swapChainCI.width = m_system->oxrViewConfigurationViews[x].recommendedImageWidth;
        swapChainCI.height = m_system->oxrViewConfigurationViews[x].recommendedImageHeight;
        swapChainCI.faceCount = 1;
        swapChainCI.arraySize = 1;
        swapChainCI.mipCount = 1;

        result = xrCreateSwapchain(m_xrSession, &swapChainCI, &colorSwapChainInfo.swapChain);
        if (XR_FAILED(result))
        {
            m_logger->Log(Common::LogLevel::Error,
              "OpenXR::CreateSwapChains: xrCreateSwapchain failed, error: {}", GetXRErrorString(m_xrInstance, result));
            return false;
        }

        colorSwapChainInfo.swapChainFormat = swapChainCI.format;
    }

    //
    // Fetch the swap chain's image data
    //
    for (auto& colorSwapChainInfo : m_colorSwapChainInfos)
    {
        uint32_t colorSwapChainImageCount = 0;
        result = xrEnumerateSwapchainImages(colorSwapChainInfo.swapChain, 0, &colorSwapChainImageCount, nullptr);
        if (XR_FAILED(result))
        {
            m_logger->Log(Common::LogLevel::Error,
              "OpenXR::CreateSwapChains: xrEnumerateSwapchainImages failed, error: {}", GetXRErrorString(m_xrInstance, result));
            return false;
        }

        colorSwapChainInfo.xrSwapChainImages.resize(colorSwapChainImageCount);

        for (auto& swapChainImage : colorSwapChainInfo.xrSwapChainImages)
        {
            swapChainImage.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
        }

        result = xrEnumerateSwapchainImages(
            colorSwapChainInfo.swapChain,
            colorSwapChainImageCount,
            &colorSwapChainImageCount,
            (XrSwapchainImageBaseHeader*)colorSwapChainInfo.xrSwapChainImages.data()
        );
        if (XR_FAILED(result))
        {
            m_logger->Log(Common::LogLevel::Error,
              "OpenXR::CreateSwapChains: xrEnumerateSwapchainImages failed, error: {}", GetXRErrorString(m_xrInstance, result));
            return false;
        }
    }

    return true;
}

void OpenXR::DestroyXrSwapChains()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying XrSwapChains");

    for (const auto& colorSwapChainInfo : m_colorSwapChainInfos)
    {
        xrDestroySwapchain(colorSwapChainInfo.swapChain);
    }
    m_colorSwapChainInfos.clear();
}

bool OpenXR::CreateXrReferenceSpace()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Creating XrReferenceSpace");

    XrReferenceSpaceCreateInfo referenceSpaceCI{};
    referenceSpaceCI.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    referenceSpaceCI.poseInReferenceSpace = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
    const auto result = xrCreateReferenceSpace(m_xrSession, &referenceSpaceCI, &m_localSpace);

    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error,
          "OpenXR::CreateReferenceSpace: Failed to create reference space, error: {}", GetXRErrorString(m_xrInstance, result));
        return false;
    }

    return true;
}

void OpenXR::DestroyXrReferenceSpace()
{
    m_logger->Log(Common::LogLevel::Info, "OpenXR: Destroying XrReferenceSpace");

    if (m_localSpace != XR_NULL_HANDLE)
    {
        xrDestroySpace(m_localSpace);
        m_localSpace = XR_NULL_HANDLE;
    }
}

void OpenXR::ProcessEvents()
{
    XrEventDataBuffer eventData{};

    const auto XrPollEvent = [&]() -> bool {
        eventData = {};
        eventData.type = XR_TYPE_EVENT_DATA_BUFFER;
        return xrPollEvent(m_xrInstance, &eventData) == XR_SUCCESS;
    };

    while (XrPollEvent())
    {
        switch (eventData.type)
        {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
            {
                auto* pEventsLost = reinterpret_cast<XrEventDataEventsLost*>(&eventData);
                m_logger->Log(Common::LogLevel::Warning, "OpenXR::PollEvents: Lost {} events", pEventsLost->lostEventCount);
            }
            break;

            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            {
                auto* pInstanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending*>(&eventData);
                m_logger->Log(Common::LogLevel::Info, "OpenXR::PollEvents: Instance loss pending at: {}", pInstanceLossPending->lossTime);

                DestroyPostInstance();
                m_state = State::InstanceCreated;
            }
            break;

            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                auto* pSessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);

                if (pSessionStateChanged->session != m_xrSession)
                {
                    m_logger->Log(Common::LogLevel::Info, "OpenXR::PollEvents: Session state changed for unknown session");
                    continue;
                }

                m_xrSessionState = pSessionStateChanged->state;

                m_logger->Log(Common::LogLevel::Info, "OpenXR::PollEvents: Session state changed to state: {}", (uint32_t)m_xrSessionState);

                if (pSessionStateChanged->state == XR_SESSION_STATE_READY)
                {
                    m_logger->Log(Common::LogLevel::Info, "OpenXR::PollEvents: Session has become ready");

                    XrSessionBeginInfo sessionBeginInfo{};
                    sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
                    sessionBeginInfo.primaryViewConfigurationType = m_system->xrViewConfigurationType;
                    auto result = xrBeginSession(m_xrSession, &sessionBeginInfo);
                    if (XR_FAILED(result))
                    {
                        m_logger->Log(Common::LogLevel::Error,
                          "OpenXR::PollEvents: xrBeginSession failed, error: {}", GetXRErrorString(m_xrInstance, result));
                    }
                }
                else if (pSessionStateChanged->state == XR_SESSION_STATE_STOPPING)
                {
                    auto result = xrEndSession(m_xrSession);
                    if (XR_FAILED(result))
                    {
                        m_logger->Log(Common::LogLevel::Error,
                          "OpenXR::PollEvents: xrEndSession failed, error: {}", GetXRErrorString(m_xrInstance, result));
                    }
                }
                else if (pSessionStateChanged->state == XR_SESSION_STATE_EXITING ||
                         pSessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING)
                {
                    DestroyPostInstance();
                    m_state = State::InstanceCreated;
                }
            }
            break;

            default: { /* no-op */ }
        }
    }
}

bool OpenXR::IsSystemAvailable() const
{
    return m_state >= State::SystemFound;
}

bool OpenXR::IsSessionCreated() const
{
    return m_state >= State::SessionCreated;
}

std::optional<OXRSystemRequirements> OpenXR::GetSystemRequirements() const
{
    if (!m_system)
    {
        return std::nullopt;
    }

    return m_system->systemRequirements;
}

std::vector<OXRViewConfigurationView> OpenXR::GetSystemEyeConfigurationViews() const
{
    return m_system->oxrViewConfigurationViews;
}

void OpenXR::BeginFrame()
{
    // Reset our frame data
    m_frame = {};

    //
    // Call xrWaitFrame
    //
    XrFrameWaitInfo frameWaitInfo{};
    frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;

    if (XR_FAILED(xrWaitFrame(m_xrSession, &frameWaitInfo, &m_frame.xrFrameState)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::WaitAndBeginFrame: Failed to wait for frame");
    }

    //
    // Call xrBeginFrame
    //
    XrFrameBeginInfo frameBeginInfo{};
    frameBeginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;

    if (XR_FAILED(xrBeginFrame(m_xrSession, &frameBeginInfo)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::WaitAndBeginFrame: Failed to begin frame");
    }
}

void OpenXR::AcquireSwapChainImages()
{
    m_frame.viewImages.clear();

    for (const auto& colorSwapChainInfo : m_colorSwapChainInfos)
    {
        uint32_t colorImageIndex = 0;
        XrSwapchainImageAcquireInfo acquireInfo{};
        acquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        auto result = xrAcquireSwapchainImage(colorSwapChainInfo.swapChain, &acquireInfo, &colorImageIndex);
        if (XR_FAILED(result))
        {
            m_logger->Log(Common::LogLevel::Error,
                          "OpenXR::WaitAndBeginFrame: Failed to acquire swap chain image, error: {}", GetXRErrorString(m_xrInstance, result));
            return;
        }

        XrSwapchainImageWaitInfo waitInfo{};
        waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        waitInfo.timeout = XR_INFINITE_DURATION;
        result = xrWaitSwapchainImage(colorSwapChainInfo.swapChain, &waitInfo);
        if (XR_FAILED(result))
        {
            m_logger->Log(Common::LogLevel::Error,
                          "OpenXR::WaitAndBeginFrame: Failed to wait for swap chain image, error: {}", GetXRErrorString(m_xrInstance, result));
            return;
        }

        m_frame.viewImages.push_back(colorSwapChainInfo.xrSwapChainImages[colorImageIndex].image);
    }
}

void OpenXR::RefreshViewData()
{
    // Reset State
    m_frame.viewPoses.clear();
    m_frame.viewFovs.clear();

    // Fetch latest OpenXR view state
    std::vector<XrView> views(m_system->oxrViewConfigurationViews.size());
    for (auto& view : views)
    {
        view.type = XR_TYPE_VIEW;
    }

    XrViewState viewState{};
    viewState.type = XR_TYPE_VIEW_STATE;

    XrViewLocateInfo viewLocateInfo{};
    viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
    viewLocateInfo.viewConfigurationType = m_system->xrViewConfigurationType;
    viewLocateInfo.displayTime = m_frame.xrFrameState.predictedDisplayTime;
    viewLocateInfo.space = m_localSpace;
    uint32_t viewCount = 0;
    XrResult result = xrLocateViews(m_xrSession, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());
    if (XR_FAILED(result))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::GetEyeViewProjections: Failed to locate OpenXR views");
        return;
    }

    // Store latest view state
    for (const auto& view : views)
    {
        m_frame.viewPoses.push_back(view.pose);
        m_frame.viewFovs.push_back(view.fov);
    }
}

void OpenXR::ReleaseSwapChainImages()
{
    for (const auto& colorSwapChainInfo : m_colorSwapChainInfos)
    {
        XrSwapchainImageReleaseInfo releaseInfo{};
        releaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
        xrReleaseSwapchainImage(colorSwapChainInfo.swapChain, &releaseInfo);
    }

    m_frame.viewImages.clear();
}

void OpenXR::EndFrame()
{
    std::vector<XrCompositionLayerProjectionView> layerProjectionViews;

    for (unsigned int x = 0; x < m_system->oxrViewConfigurationViews.size(); ++x)
    {
        XrCompositionLayerProjectionView layerProjectionView{};
        layerProjectionView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        layerProjectionView.pose = m_frame.viewPoses.at(x);
        layerProjectionView.fov = m_frame.viewFovs.at(x);
        layerProjectionView.subImage.swapchain = m_colorSwapChainInfos.at(x).swapChain;
        layerProjectionView.subImage.imageRect.offset.x = 0;
        layerProjectionView.subImage.imageRect.offset.y = 0;
        layerProjectionView.subImage.imageRect.extent.width = static_cast<int32_t>(m_system->oxrViewConfigurationViews.at(x).recommendedImageWidth);
        layerProjectionView.subImage.imageRect.extent.height = static_cast<int32_t>(m_system->oxrViewConfigurationViews.at(x).recommendedImageHeight);
        layerProjectionView.subImage.imageArrayIndex = 0;

        layerProjectionViews.push_back(layerProjectionView);
    }

    XrCompositionLayerProjection layerProjection{};
    layerProjection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    layerProjection.space = m_localSpace;
    layerProjection.viewCount = static_cast<uint32_t>(layerProjectionViews.size());
    layerProjection.views = layerProjectionViews.data();

    std::vector<XrCompositionLayerBaseHeader*> layers;
    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProjection));

    XrFrameEndInfo frameEndInfo{};
    frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
    frameEndInfo.displayTime = m_frame.xrFrameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
    frameEndInfo.layers = layers.data();

    if (XR_FAILED(xrEndFrame(m_xrSession, &frameEndInfo)))
    {
        m_logger->Log(Common::LogLevel::Error, "OpenXR::EndFrame: Failed to end frame");
    }
}

std::vector<OXREyeView> OpenXR::GetFrameEyeViews() const
{
    std::vector<OXREyeView> eyeViews;

    for (unsigned int x = 0; x < m_system->oxrViewConfigurationViews.size(); ++x)
    {
        const auto& eyePose = m_frame.viewPoses[x];
        const auto& eyeFov = m_frame.viewFovs[x];

        // Pose
        const auto posePosition = FromOpenRX(eyePose.position);
        const auto poseOrientation = FromOpenRX(eyePose.orientation);

        // Fov
        const auto leftTanHalfAngle = glm::tan(eyeFov.angleLeft);
        const auto rightTanHalfAngle = glm::tan(eyeFov.angleRight);
        const auto upTanHalfAngle = glm::tan(eyeFov.angleUp);
        const auto downTanHalfAngle = glm::tan(eyeFov.angleDown);

        eyeViews.push_back(OXREyeView{
            .posePosition = posePosition,
            .poseOrientation = poseOrientation,
            .leftTanHalfAngle = leftTanHalfAngle,
            .rightTanHalfAngle = rightTanHalfAngle,
            .upTanHalfAngle = upTanHalfAngle,
            .downTanHalfAngle = downTanHalfAngle}
        );
    }

    return eyeViews;
}

VkImage OpenXR::GetFrameEyeImage(const Eye& eye) const
{
    return m_frame.viewImages.at(static_cast<unsigned int>(eye));
}

}
