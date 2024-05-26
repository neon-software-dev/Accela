#include "VulkanInstance.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>

#include <format>
#include <cstring>
#include <set>
#include <algorithm>
#include <functional>

namespace Accela::Render
{

VulkanInstance::VulkanInstance(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_vulkanContext(std::move(vulkanContext))
{

}

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                      void* pUserData)
{
    (void)messageType;

    auto* pLogger = (Common::ILogger*)pUserData;

    Common::LogLevel logLevel;

    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logLevel = Common::LogLevel::Info; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logLevel = Common::LogLevel::Warning; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logLevel = Common::LogLevel::Error; break;
        default: logLevel = Common::LogLevel::Debug;
    }

    // Override performance warnings to debug level, mostly because OpenVR absolutely spams us with
    // performance warnings that we can't fix, so dropping the severity down
    if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        logLevel = Common::LogLevel::Debug;
    }

    pLogger->Log(logLevel, "[VulkanMessage] {}", pCallbackData->pMessage);

    return VK_FALSE; // Note the spec says to always return false
}

bool VulkanInstance::CreateInstance(const std::string& appName, uint32_t appVersion, bool enableValidationLayers)
{
    //
    // Verify that the system supports the version of Vulkan we require
    //
    if (!VerifyVulkanVersion())
    {
        return false;
    }

    //
    // Query for the extensions the Vulkan context requires
    //
    std::set<std::string> requiredExtensions;
    if (!m_vulkanContext->GetRequiredInstanceExtensions(requiredExtensions))
    {
        m_logger->Log(Common::LogLevel::Error, "CreateInstance: Failed to fetch required Vulkan extensions");
        return false;
    }

    //
    // Verify that the required extensions are available, either globally, or by an installed layer
    //
    const auto instanceProperties = GatherInstanceProperties();

    std::set<std::string> extensions;
    std::set<std::string> layers;

    for (const auto& requiredExtension : requiredExtensions)
    {
        // See if the Vulkan instance/driver provides support for the extension
        if (InstanceSupportsExtension(instanceProperties, requiredExtension))
        {
            extensions.insert(requiredExtension);
            continue;
        }

        // If not, see if any installed layers provides support for the extension
        const auto layer = FindLayerSupportingExtension(instanceProperties, requiredExtension);
        if (layer.has_value())
        {
            extensions.insert(requiredExtension); // Use the extension
            layers.insert(*layer); // And use the layer that supplies the extension
            continue;
        }

        m_logger->Log(Common::LogLevel::Fatal,
            "Unable to find support for required extension: {}", requiredExtension);
        return false;
    }

    //
    // Special case handling of enabling validation layer + debug extension
    //
    bool usingValidationLayers = false;

    if (enableValidationLayers)
    {
        const bool validationLayerAvailable = InstanceSupportsLayer(instanceProperties, "VK_LAYER_KHRONOS_validation");
        const bool debugExtensionGloballyProvided = InstanceSupportsExtension(instanceProperties, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        const auto debugExtensionLayerProvided = FindLayerSupportingExtension(instanceProperties, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        if (!validationLayerAvailable)
        {
            m_logger->Log(Common::LogLevel::Warning, "Requested validation layer but the layer isn't supported");
        }
        else
        {
            if (debugExtensionGloballyProvided)
            {
                extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                layers.insert("VK_LAYER_KHRONOS_validation");
                usingValidationLayers = true;
            }
            else if (debugExtensionLayerProvided.has_value())
            {
                extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                layers.insert("VK_LAYER_KHRONOS_validation");
                layers.insert(*debugExtensionLayerProvided);
                usingValidationLayers = true;
            }
            else
            {
                m_logger->Log(Common::LogLevel::Warning,
                  "Requested validation layer but the debug extension isn't supported");
            }
        }
    }

    //
    // Configure and create the Vulkan instance
    //
    std::vector<const char*> extensionsCStrs;
    std::transform(std::begin(extensions), std::end(extensions),
                   std::back_inserter(extensionsCStrs), std::mem_fn(&std::string::c_str));

    std::vector<const char*> layersCStrs;
    std::transform(std::begin(layers), std::end(layers),
                   std::back_inserter(layersCStrs), std::mem_fn(&std::string::c_str));

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.pfnUserCallback = VkDebugCallback;
    debugMessengerCreateInfo.pUserData = (void*)m_logger.get();

    const std::string engineName{"ACCELA"};
    const uint32_t engineVersion = 1;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName.c_str();
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = engineName.c_str();
    appInfo.engineVersion = engineVersion;
    appInfo.apiVersion = VULKAN_API_VERSION;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionsCStrs.size();
    createInfo.ppEnabledExtensionNames = extensionsCStrs.data();
    createInfo.enabledLayerCount = layersCStrs.size();
    createInfo.ppEnabledLayerNames = layersCStrs.data();

    if (usingValidationLayers)
    {
        m_logger->Log(Common::LogLevel::Info, "CreateInstance: Using validation layers");
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
    }

    const VkResult result = m_vulkanCalls->vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateInstance: vkCreateInstance call failure, result code: {}", (uint32_t)result);
        return false;
    }

    //
    // Now that an instance is created, load the Vulkan instance calls
    //
    if (!m_vulkanCalls->InitInstanceCalls(m_vkInstance))
    {
        m_logger->Log(Common::LogLevel::Error, "CreateInstance: Failed to init instance calls");
        Destroy();
        return false;
    }

    //
    // Set up the debug messenger, if needed
    //
    if (usingValidationLayers)
    {
        if (m_vulkanCalls->vkCreateDebugUtilsMessengerEXT(m_vkInstance, &debugMessengerCreateInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS)
        {
            m_logger->Log(Common::LogLevel::Warning, "CreateInstance: vkCreateDebugUtilsMessengerEXT failed");
        }
    }

    return true;
}

void VulkanInstance::Destroy() noexcept
{
    if (m_vkDebugMessenger != VK_NULL_HANDLE)
    {
        m_vulkanCalls->vkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
        m_vkDebugMessenger = VK_NULL_HANDLE;
    }

    if (m_vkInstance != VK_NULL_HANDLE)
    {
        m_vulkanCalls->vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = VK_NULL_HANDLE;
    }
}

bool VulkanInstance::VerifyVulkanVersion() const
{
    uint32_t queriedApiVersion{0};
    if (m_vulkanCalls->vkEnumerateInstanceVersion(&queriedApiVersion) != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "VerifyVulkanVersion: Failed to query for Vulkan version");
        return false;
    }

    const std::string queriedApiVersionStr =
        std::format("{}.{}.{}.{}",
            VK_API_VERSION_VARIANT(queriedApiVersion),
            VK_API_VERSION_MAJOR(queriedApiVersion),
            VK_API_VERSION_MINOR(queriedApiVersion),
            VK_API_VERSION_PATCH(queriedApiVersion)
        );

    if (queriedApiVersion < VULKAN_API_VERSION)
    {
        m_logger->Log(Common::LogLevel::Error,
          "VerifyVulkanVersion: Unsupported Vulkan version: {}", queriedApiVersionStr);
        return false;
    }
    else
    {
        m_logger->Log(Common::LogLevel::Info,
          "VerifyVulkanVersion: Found supported Vulkan version: {}", queriedApiVersionStr);
    }

    return true;
}

VulkanInstance::InstanceProperties VulkanInstance::GatherInstanceProperties() const
{
    VulkanInstance::InstanceProperties result;

    uint32_t availableExtensionsCount = 0;
    m_vulkanCalls->vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
    m_vulkanCalls->vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());

    for (const auto& extension : availableExtensions)
    {
        result.extensions.push_back(extension);
    }

    uint32_t availableLayersCount = 0;
    m_vulkanCalls->vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableLayersCount);
    m_vulkanCalls->vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());

    for (const auto& layer : availableLayers)
    {
        LayerProperties layerProperties;
        layerProperties.layerName = layer.layerName;

        uint32_t layerAvailableExtensionsCount = 0;
        m_vulkanCalls->vkEnumerateInstanceExtensionProperties(layer.layerName, &layerAvailableExtensionsCount, nullptr);

        std::vector<VkExtensionProperties> layerAvailableExtensions(layerAvailableExtensionsCount);
        m_vulkanCalls->vkEnumerateInstanceExtensionProperties(layer.layerName, &layerAvailableExtensionsCount, layerAvailableExtensions.data());

        for (const auto& layerExtension : layerAvailableExtensions)
        {
            layerProperties.extensions.push_back(layerExtension);
        }

        result.layers.push_back(layerProperties);
    }

    return result;
}

bool VulkanInstance::InstanceSupportsExtension(const InstanceProperties& properties, const std::string& extensionName)
{
    return std::any_of(properties.extensions.begin(), properties.extensions.end(), [&](const VkExtensionProperties& extension){
        return strcmp(extension.extensionName, extensionName.c_str()) == 0;
    });
}

std::optional<std::string>
VulkanInstance::FindLayerSupportingExtension(const VulkanInstance::InstanceProperties& properties,
                                             const std::string& extensionName)
{
    for (const auto& layerProperties : properties.layers)
    {
        bool supportsExtension = std::any_of(layerProperties.extensions.begin(), layerProperties.extensions.end(),
        [&](const VkExtensionProperties& extension){
            return strcmp(extension.extensionName, extensionName.c_str()) == 0;
        });

        if (supportsExtension)
        {
            return layerProperties.layerName;
        }
    }

    return std::nullopt;
}

bool VulkanInstance::InstanceSupportsLayer(const VulkanInstance::InstanceProperties& properties,
                                           const std::string& layerName)
{
    return std::any_of(properties.layers.begin(), properties.layers.end(), [&](const LayerProperties& layer){
        return strcmp(layer.layerName.c_str(), layerName.c_str()) == 0;
    });
}

}
