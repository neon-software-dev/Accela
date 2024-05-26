#include <Accela/Render/VulkanCalls.h>

namespace Accela::Render
{

#define FIND_GLOBAL_CALL(c) m_##c = (PFN_##c)m_pVkGetInstanceProcAddr(nullptr, #c);
#define FIND_INSTANCE_CALL(c) m_##c = (PFN_##c)m_pVkGetInstanceProcAddr(vkInstance, #c);
#define FIND_DEVICE_CALL(c) m_##c = (PFN_##c)m_pVkGetDeviceProcAddr(vkDevice, #c);

bool VulkanCalls::InitGlobalCalls()
{
    m_pVkGetInstanceProcAddr = GetInstanceProcAddrFunc();
    if (m_pVkGetInstanceProcAddr == nullptr)
    {
        return false;
    }

    FIND_GLOBAL_CALL(vkCreateInstance)
    FIND_GLOBAL_CALL(vkEnumerateInstanceLayerProperties)
    FIND_GLOBAL_CALL(vkEnumerateInstanceExtensionProperties)
    FIND_GLOBAL_CALL(vkEnumerateInstanceVersion)

    return true;
}

bool VulkanCalls::InitInstanceCalls(VkInstance vkInstance)
{
    m_pVkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)m_pVkGetInstanceProcAddr(vkInstance, "vkGetDeviceProcAddr");
    if (m_pVkGetDeviceProcAddr == nullptr)
    {
        return false;
    }

    FIND_INSTANCE_CALL(vkCreateDebugUtilsMessengerEXT)
    FIND_INSTANCE_CALL(vkDestroyDebugUtilsMessengerEXT)
    FIND_INSTANCE_CALL(vkCmdBeginDebugUtilsLabelEXT)
    FIND_INSTANCE_CALL(vkCmdEndDebugUtilsLabelEXT)
    FIND_INSTANCE_CALL(vkCmdInsertDebugUtilsLabelEXT)
    FIND_INSTANCE_CALL(vkQueueBeginDebugUtilsLabelEXT)
    FIND_INSTANCE_CALL(vkQueueEndDebugUtilsLabelEXT)
    FIND_INSTANCE_CALL(vkDestroyInstance)
    FIND_INSTANCE_CALL(vkEnumeratePhysicalDevices)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceProperties)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceFeatures)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceFeatures2)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceQueueFamilyProperties)
    FIND_INSTANCE_CALL(vkCreateDevice)
    FIND_INSTANCE_CALL(vkDestroySurfaceKHR)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceSurfaceSupportKHR)
    FIND_INSTANCE_CALL(vkEnumerateDeviceExtensionProperties)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceMemoryProperties)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceMemoryProperties2)
    FIND_INSTANCE_CALL(vkGetPhysicalDeviceFormatProperties)

    return true;
}

bool VulkanCalls::InitDeviceCalls(VkDevice vkDevice)
{
    FIND_DEVICE_CALL(vkSetDebugUtilsObjectNameEXT)
    FIND_DEVICE_CALL(vkDestroyDevice)
    FIND_DEVICE_CALL(vkGetDeviceQueue)
    FIND_DEVICE_CALL(vkCreateSwapchainKHR)
    FIND_DEVICE_CALL(vkDestroySwapchainKHR)
    FIND_DEVICE_CALL(vkGetSwapchainImagesKHR)
    FIND_DEVICE_CALL(vkCreateImageView)
    FIND_DEVICE_CALL(vkDestroyImageView)
    FIND_DEVICE_CALL(vkCreateShaderModule)
    FIND_DEVICE_CALL(vkDestroyShaderModule)
    FIND_DEVICE_CALL(vkCreatePipelineLayout)
    FIND_DEVICE_CALL(vkDestroyPipelineLayout)
    FIND_DEVICE_CALL(vkCreateRenderPass)
    FIND_DEVICE_CALL(vkDestroyRenderPass)
    FIND_DEVICE_CALL(vkCreateGraphicsPipelines)
    FIND_DEVICE_CALL(vkDestroyPipeline)
    FIND_DEVICE_CALL(vkCreateFramebuffer)
    FIND_DEVICE_CALL(vkDestroyFramebuffer)
    FIND_DEVICE_CALL(vkCreateCommandPool)
    FIND_DEVICE_CALL(vkDestroyCommandPool)
    FIND_DEVICE_CALL(vkAllocateCommandBuffers)
    FIND_DEVICE_CALL(vkBeginCommandBuffer)
    FIND_DEVICE_CALL(vkCmdBeginRenderPass)
    FIND_DEVICE_CALL(vkCmdNextSubpass)
    FIND_DEVICE_CALL(vkCmdBindPipeline)
    FIND_DEVICE_CALL(vkCmdBindVertexBuffers)
    FIND_DEVICE_CALL(vkCmdBindIndexBuffer)
    FIND_DEVICE_CALL(vkCmdDraw)
    FIND_DEVICE_CALL(vkCmdDrawIndexed)
    FIND_DEVICE_CALL(vkCmdEndRenderPass)
    FIND_DEVICE_CALL(vkEndCommandBuffer)
    FIND_DEVICE_CALL(vkCreateSemaphore)
    FIND_DEVICE_CALL(vkDestroySemaphore)
    FIND_DEVICE_CALL(vkAcquireNextImageKHR)
    FIND_DEVICE_CALL(vkQueueSubmit)
    FIND_DEVICE_CALL(vkQueuePresentKHR)
    FIND_DEVICE_CALL(vkQueueWaitIdle)
    FIND_DEVICE_CALL(vkDeviceWaitIdle)
    FIND_DEVICE_CALL(vkResetCommandBuffer)
    FIND_DEVICE_CALL(vkResetCommandPool)
    FIND_DEVICE_CALL(vkCreateFence)
    FIND_DEVICE_CALL(vkWaitForFences)
    FIND_DEVICE_CALL(vkResetFences)
    FIND_DEVICE_CALL(vkDestroyFence)
    FIND_DEVICE_CALL(vkAllocateMemory)
    FIND_DEVICE_CALL(vkFreeMemory)
    FIND_DEVICE_CALL(vkMapMemory)
    FIND_DEVICE_CALL(vkUnmapMemory)
    FIND_DEVICE_CALL(vkFlushMappedMemoryRanges)
    FIND_DEVICE_CALL(vkInvalidateMappedMemoryRanges)
    FIND_DEVICE_CALL(vkBindBufferMemory)
    FIND_DEVICE_CALL(vkBindImageMemory)
    FIND_DEVICE_CALL(vkGetBufferMemoryRequirements)
    FIND_DEVICE_CALL(vkGetImageMemoryRequirements)
    FIND_DEVICE_CALL(vkCreateBuffer)
    FIND_DEVICE_CALL(vkDestroyBuffer)
    FIND_DEVICE_CALL(vkCreateImage)
    FIND_DEVICE_CALL(vkDestroyImage)
    FIND_DEVICE_CALL(vkCmdCopyBuffer)
    FIND_DEVICE_CALL(vkGetBufferMemoryRequirements2)
    FIND_DEVICE_CALL(vkGetImageMemoryRequirements2)
    FIND_DEVICE_CALL(vkBindBufferMemory2)
    FIND_DEVICE_CALL(vkBindImageMemory2)
    FIND_DEVICE_CALL(vkCmdPushConstants)
    FIND_DEVICE_CALL(vkCreateDescriptorPool)
    FIND_DEVICE_CALL(vkDestroyDescriptorPool)
    FIND_DEVICE_CALL(vkCreateDescriptorSetLayout)
    FIND_DEVICE_CALL(vkDestroyDescriptorSetLayout)
    FIND_DEVICE_CALL(vkAllocateDescriptorSets)
    FIND_DEVICE_CALL(vkUpdateDescriptorSets)
    FIND_DEVICE_CALL(vkCmdBindDescriptorSets)
    FIND_DEVICE_CALL(vkCmdPipelineBarrier)
    FIND_DEVICE_CALL(vkCmdCopyBufferToImage)
    FIND_DEVICE_CALL(vkResetDescriptorPool)
    FIND_DEVICE_CALL(vkCreateSampler)
    FIND_DEVICE_CALL(vkDestroySampler)
    FIND_DEVICE_CALL(vkFreeCommandBuffers)
    FIND_DEVICE_CALL(vkGetFenceStatus)
    FIND_DEVICE_CALL(vkFreeDescriptorSets)
    FIND_DEVICE_CALL(vkCmdCopyImage)
    FIND_DEVICE_CALL(vkCmdSetViewport)
    FIND_DEVICE_CALL(vkCmdClearAttachments)
    FIND_DEVICE_CALL(vkCmdBlitImage)

    return true;
}

VmaFuncs VulkanCalls::GetVMAFuncs() const
{
    VmaFuncs funcs{};

    funcs.vkGetPhysicalDeviceProperties = m_vkGetPhysicalDeviceProperties;
    funcs.vkGetPhysicalDeviceMemoryProperties = m_vkGetPhysicalDeviceMemoryProperties;
    funcs.vkAllocateMemory = m_vkAllocateMemory;
    funcs.vkFreeMemory = m_vkFreeMemory;
    funcs.vkMapMemory = m_vkMapMemory;
    funcs.vkUnmapMemory = m_vkUnmapMemory;
    funcs.vkFlushMappedMemoryRanges = m_vkFlushMappedMemoryRanges;
    funcs.vkInvalidateMappedMemoryRanges = m_vkInvalidateMappedMemoryRanges;
    funcs.vkBindBufferMemory = m_vkBindBufferMemory;
    funcs.vkBindImageMemory = m_vkBindImageMemory;
    funcs.vkGetBufferMemoryRequirements = m_vkGetBufferMemoryRequirements;
    funcs.vkGetImageMemoryRequirements = m_vkGetImageMemoryRequirements;
    funcs.vkCreateBuffer = m_vkCreateBuffer;
    funcs.vkDestroyBuffer = m_vkDestroyBuffer;
    funcs.vkCreateImage = m_vkCreateImage;
    funcs.vkDestroyImage = m_vkDestroyImage;
    funcs.vkCmdCopyBuffer = m_vkCmdCopyBuffer;
    funcs.vkGetBufferMemoryRequirements2KHR = m_vkGetBufferMemoryRequirements2;
    funcs.vkGetImageMemoryRequirements2KHR = m_vkGetImageMemoryRequirements2;
    funcs.vkBindBufferMemory2KHR = m_vkBindBufferMemory2;
    funcs.vkBindImageMemory2KHR = m_vkBindImageMemory2;
    funcs.vkGetPhysicalDeviceMemoryProperties2KHR = m_vkGetPhysicalDeviceMemoryProperties2;

    return funcs;
}

VkResult VulkanCalls::vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                       VkInstance *pInstance) const
{
    return m_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VkResult VulkanCalls::vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) const
{
    return m_vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult VulkanCalls::vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount,
                                                             VkExtensionProperties *pProperties) const
{
    return m_vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VkResult VulkanCalls::vkEnumerateInstanceVersion(uint32_t* pApiVersion) const
{
    return m_vkEnumerateInstanceVersion(pApiVersion);
}

VkResult
VulkanCalls::vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator,
                                            VkDebugUtilsMessengerEXT *pMessenger) const
{
    return m_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

void VulkanCalls::vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                  const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

void
VulkanCalls::vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo) const
{
    if (m_vkCmdBeginDebugUtilsLabelEXT == nullptr) { return; }

    return m_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void VulkanCalls::vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) const
{
    if (m_vkCmdEndDebugUtilsLabelEXT == nullptr) { return; }

    return m_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void VulkanCalls::vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) const
{
    return m_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

void VulkanCalls::vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo) const
{
    if (m_vkQueueBeginDebugUtilsLabelEXT == nullptr) { return; }

    return m_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
}

void VulkanCalls::vkQueueEndDebugUtilsLabelEXT(VkQueue queue) const
{
    if (m_vkQueueEndDebugUtilsLabelEXT == nullptr) { return; }

    return m_vkQueueEndDebugUtilsLabelEXT(queue);
}

void VulkanCalls::vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyInstance(instance, pAllocator);
}

VkResult VulkanCalls::vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                 VkPhysicalDevice *pPhysicalDevices) const
{
    return m_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

void VulkanCalls::vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                                VkPhysicalDeviceProperties *pProperties) const
{
    return m_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void VulkanCalls::vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) const
{
    return m_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void VulkanCalls::vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures) const
{
    return m_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

void VulkanCalls::vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                           uint32_t *pQueueFamilyPropertyCount,
                                                           VkQueueFamilyProperties *pQueueFamilyProperties) const
{
    return m_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VkResult VulkanCalls::vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) const
{
    return m_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void VulkanCalls::vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                                      const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VkResult VulkanCalls::vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                           VkSurfaceKHR surface, VkBool32 *pSupported) const
{
    return m_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult VulkanCalls::vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                           uint32_t *pPropertyCount,
                                                           VkExtensionProperties *pProperties) const
{
    return m_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult VulkanCalls::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) const
{
    return m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

VkResult VulkanCalls::vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                           uint32_t *pSurfaceFormatCount,
                                                           VkSurfaceFormatKHR *pSurfaceFormats) const
{
    return m_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult VulkanCalls::vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                uint32_t *pPresentModeCount,
                                                                VkPresentModeKHR *pPresentModes) const
{
    return m_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

void VulkanCalls::vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                                      VkPhysicalDeviceMemoryProperties *pMemoryProperties) const
{
    return m_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void VulkanCalls::vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
                                                       VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) const
{
    return m_vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void VulkanCalls::vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                      VkFormatProperties *pFormatProperties) const
{
    return m_vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VkResult
VulkanCalls::vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo) const
{
    if (m_vkSetDebugUtilsObjectNameEXT == nullptr) { return VK_SUCCESS; }

    return m_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

void VulkanCalls::vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyDevice(device, pAllocator);
}

void
VulkanCalls::vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) const
{
    return m_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult VulkanCalls::vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) const
{
    return m_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

void VulkanCalls::vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                                        const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult VulkanCalls::vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                              VkImage *pSwapchainImages) const
{
    return m_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult VulkanCalls::vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                        const VkAllocationCallbacks *pAllocator, VkImageView *pView) const
{
    return m_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

void
VulkanCalls::vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyImageView(device, imageView, pAllocator);
}

VkResult VulkanCalls::vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) const
{
    return m_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

void VulkanCalls::vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                                        const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyShaderModule(device, shaderModule, pAllocator);
}

VkResult VulkanCalls::vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkPipelineLayout *pPipelineLayout) const
{
    return m_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

void VulkanCalls::vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                                          const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

VkResult VulkanCalls::vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                         const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) const
{
    return m_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

void VulkanCalls::vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass,
                                      const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyRenderPass(device, renderPass, pAllocator);
}

VkResult
VulkanCalls::vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                       const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) const
{
    return m_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

void VulkanCalls::vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyPipeline(device, pipeline, pAllocator);
}

VkResult VulkanCalls::vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) const
{
    return m_vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

void VulkanCalls::vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer,
                                       const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

VkResult VulkanCalls::vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) const
{
    return m_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

void VulkanCalls::vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                                       const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkResult VulkanCalls::vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                               VkCommandBuffer *pCommandBuffers) const
{
    return m_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

void VulkanCalls::vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                       const VkCommandBuffer *pCommandBuffers) const
{
    return m_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult
VulkanCalls::vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) const
{
    return m_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

void VulkanCalls::vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                       VkSubpassContents contents) const
{
    return m_vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void VulkanCalls::vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) const
{
    m_vkCmdNextSubpass(commandBuffer, contents);
}

void VulkanCalls::vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                    VkPipeline pipeline) const
{
    return m_vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void VulkanCalls::vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                         const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) const
{
    return m_vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void VulkanCalls::vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                       VkIndexType indexType) const
{
    return m_vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void VulkanCalls::vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                            uint32_t firstVertex, uint32_t firstInstance) const
{
    return m_vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCalls::vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
{
    return m_vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCalls::vkCmdEndRenderPass(VkCommandBuffer commandBuffer) const
{
    return m_vkCmdEndRenderPass(commandBuffer);
}

VkResult VulkanCalls::vkEndCommandBuffer(VkCommandBuffer commandBuffer) const
{
    return m_vkEndCommandBuffer(commandBuffer);
}

VkResult VulkanCalls::vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                        const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) const
{
    return m_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

void
VulkanCalls::vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroySemaphore(device, semaphore, pAllocator);
}

VkResult
VulkanCalls::vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore,
                                   VkFence fence, uint32_t *pImageIndex) const
{
    return m_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult
VulkanCalls::vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) const
{
    return m_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult VulkanCalls::vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) const
{
    return m_vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult VulkanCalls::vkQueueWaitIdle(VkQueue queue) const
{
    return m_vkQueueWaitIdle(queue);
}

VkResult VulkanCalls::vkDeviceWaitIdle(VkDevice device) const
{
    return m_vkDeviceWaitIdle(device);
}

VkResult VulkanCalls::vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) const
{
    return m_vkResetCommandBuffer(commandBuffer, flags);
}

VkResult
VulkanCalls::vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) const
{
    return m_vkResetCommandPool(device, commandPool, flags);
}

VkResult VulkanCalls::vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
                                    const VkAllocationCallbacks *pAllocator, VkFence *pFence) const
{
    return m_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

VkResult VulkanCalls::vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll,
                                      uint64_t timeout) const
{
    return m_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

VkResult VulkanCalls::vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) const
{
    return m_vkResetFences(device, fenceCount, pFences);
}

void VulkanCalls::vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyFence(device, fence, pAllocator);
}

VkResult VulkanCalls::vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                       const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) const
{
    return m_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

void VulkanCalls::vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkFreeMemory(device, memory, pAllocator);
}

VkResult VulkanCalls::vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
                                  VkMemoryMapFlags flags, void **ppData) const
{
    return m_vkMapMemory(device, memory, offset, size, flags, ppData);
}

void VulkanCalls::vkUnmapMemory(VkDevice device, VkDeviceMemory memory) const
{
    return m_vkUnmapMemory(device, memory);
}

VkResult VulkanCalls::vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                const VkMappedMemoryRange *pMemoryRanges) const
{
    return m_vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult VulkanCalls::vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                     const VkMappedMemoryRange *pMemoryRanges) const
{
    return m_vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult VulkanCalls::vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory,
                                         VkDeviceSize memoryOffset) const
{
    return m_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult
VulkanCalls::vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) const
{
    return m_vkBindImageMemory(device, image, memory, memoryOffset);
}

void VulkanCalls::vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                                VkMemoryRequirements *pMemoryRequirements) const
{
    return m_vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void VulkanCalls::vkGetImageMemoryRequirements(VkDevice device, VkImage image,
                                               VkMemoryRequirements *pMemoryRequirements) const
{
    return m_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

VkResult VulkanCalls::vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) const
{
    return m_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

void VulkanCalls::vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyBuffer(device, buffer, pAllocator);
}

VkResult VulkanCalls::vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                    const VkAllocationCallbacks *pAllocator, VkImage *pImage) const
{
    return m_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

void VulkanCalls::vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyImage(device, image, pAllocator);
}

void VulkanCalls::vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                  uint32_t regionCount, const VkBufferCopy *pRegions) const
{
    return m_vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void VulkanCalls::vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo,
                                                 VkMemoryRequirements2 *pMemoryRequirements) const
{
    return m_vkGetBufferMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

void VulkanCalls::vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo,
                                                VkMemoryRequirements2 *pMemoryRequirements) const
{
    return m_vkGetImageMemoryRequirements2(device, pInfo, pMemoryRequirements);
}

VkResult VulkanCalls::vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount,
                                          const VkBindBufferMemoryInfo *pBindInfos) const
{
    return m_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult
VulkanCalls::vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos) const
{
    return m_vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

void
VulkanCalls::vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                uint32_t offset, uint32_t size, const void *pValues) const
{
    return m_vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VkResult VulkanCalls::vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator,
                                             VkDescriptorPool *pDescriptorPool) const
{
    return m_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

void VulkanCalls::vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                          const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

VkResult VulkanCalls::vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkDescriptorSetLayout *pSetLayout) const
{
    return m_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

void VulkanCalls::vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                               const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

VkResult VulkanCalls::vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                               VkDescriptorSet *pDescriptorSets) const
{
    return m_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

void VulkanCalls::vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                         const VkCopyDescriptorSet *pDescriptorCopies) const
{
    return m_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

void VulkanCalls::vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                          VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                          const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
                                          const uint32_t *pDynamicOffsets) const
{
    return m_vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void VulkanCalls::vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                       uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                       uint32_t bufferMemoryBarrierCount,
                                       const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                       uint32_t imageMemoryBarrierCount,
                                       const VkImageMemoryBarrier *pImageMemoryBarriers) const
{
    return m_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void VulkanCalls::vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                         VkImageLayout dstImageLayout, uint32_t regionCount,
                                         const VkBufferImageCopy *pRegions) const
{
    return m_vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VkResult VulkanCalls::vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                            VkDescriptorPoolResetFlags flags) const
{
    return m_vkResetDescriptorPool(device, descriptorPool, flags);
}

VkResult VulkanCalls::vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) const
{
    return m_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

void VulkanCalls::vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) const
{
    return m_vkDestroySampler(device, sampler, pAllocator);
}

VkResult VulkanCalls::vkGetFenceStatus(VkDevice device, VkFence fence) const
{
    return m_vkGetFenceStatus(device, fence);
}

VkResult
VulkanCalls::vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                                  const VkDescriptorSet *pDescriptorSets) const
{
    return m_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void VulkanCalls::vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) const
{
    return m_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VulkanCalls::vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const
{
    m_vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void VulkanCalls::vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                        const VkClearAttachment *pAttachments, uint32_t rectCount,
                                        const VkClearRect *pRects) const
{
    m_vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

void VulkanCalls::vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) const
{
    m_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

}
