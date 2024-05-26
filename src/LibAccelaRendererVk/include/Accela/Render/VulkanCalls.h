#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VULKANCALLS_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VULKANCALLS_H

#include "IVulkanCalls.h"

namespace Accela::Render
{
    /**
     * Provides access to all Vulkan API calls. Resolves calls from an initial PFN_vkGetInstanceProcAddr
     * function pointer provided which is provided by the concrete subclass.
     */
    class VulkanCalls : public IVulkanCalls
    {
        public:

            ~VulkanCalls() override = default;

            bool InitGlobalCalls() override;
            bool InitInstanceCalls(VkInstance vkInstance) override;
            bool InitDeviceCalls(VkDevice vkDevice) override;
            [[nodiscard]] VmaFuncs GetVMAFuncs() const override;

        public:

            //
            // Global Calls
            //
            VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) const override;
            VkResult vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) const override;
            VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const override;
            VkResult vkEnumerateInstanceVersion(uint32_t* pApiVersion) const override;

            //
            // Instance Calls
            //
            VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) const override;
            void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator) const override;
            void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) const override;
            void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) const override;
            void vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) const override;
            void vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo) const override;
            void vkQueueEndDebugUtilsLabelEXT(VkQueue queue) const override;
            void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const override;
            void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) const override;
            void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) const override;
            void vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures) const override;
            void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) const override;
            VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) const override;
            void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) const override;
            VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const override;
            VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const override;
            VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) const override;
            VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) const override;
            void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) const override;
            void vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2* pMemoryProperties) const override;
            void vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) const override;

            //
            // Device calls
            //
            VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) const override;
            void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) const override;
            void vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) const override;
            VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) const override;
            void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) const override;
            VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) const override;
            void vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) const override;
            void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) const override;
            void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) const override;
            void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const override;
            void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) const override;
            void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) const override;
            void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) const override;
            void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const override;
            VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) const override;
            void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) const override;
            void vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) const override;
            void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const override;
            void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const override;
            void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const override;
            void vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const override;
            void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const override;
            void vkCmdEndRenderPass(VkCommandBuffer commandBuffer) const override;
            VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer) const override;
            VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) const override;
            void vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) const override;
            VkResult vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) const override;
            VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) const override;
            VkResult vkQueueWaitIdle(VkQueue queue) const override;
            VkResult vkDeviceWaitIdle(VkDevice device) const override;
            VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) const override;
            VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) const override;
            VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const override;
            VkResult vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) const override;
            VkResult vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences) const override;
            void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) const override;
            void vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) const override;
            void vkUnmapMemory(VkDevice device, VkDeviceMemory memory) const override;
            VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const override;
            VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const override;
            VkResult vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) const override;
            VkResult vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) const override;
            void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) const override;
            void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) const override;
            VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) const override;
            void vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) const override;
            void vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) const override;
            void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const override;
            void vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) const override;
            void vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2* pInfo, VkMemoryRequirements2* pMemoryRequirements) const override;
            VkResult vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) const override;
            VkResult vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) const override;
            void vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues) const override;
            VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) const override;
            void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) const override;
            void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) const override;
            void vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) const override;
            void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const override;
            void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) const override;
            void vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const override;
            VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) const override;
            VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) const override;
            void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) const override;
            VkResult vkGetFenceStatus(VkDevice device, VkFence fence) const override;
            VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const override;
            void vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) const override;
            void vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const override;
            void vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) const override;
            void vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) const override;

        protected:

            /**
             * @return The vkGetInstanceProcAddr function, or nullptr on error
             */
            virtual PFN_vkGetInstanceProcAddr GetInstanceProcAddrFunc() = 0;

        protected:

            //
            // Lookup Calls
            //
            // Note the intentional slight naming difference vs the others, so they can be
            // easily referenced separately from the actual vk functions
            //
            PFN_vkGetInstanceProcAddr m_pVkGetInstanceProcAddr{nullptr};
            PFN_vkGetDeviceProcAddr m_pVkGetDeviceProcAddr{nullptr};

        public:

            //
            // Global calls function pointers
            //
            PFN_vkCreateInstance m_vkCreateInstance{nullptr};
            PFN_vkEnumerateInstanceLayerProperties m_vkEnumerateInstanceLayerProperties{nullptr};
            PFN_vkEnumerateInstanceExtensionProperties m_vkEnumerateInstanceExtensionProperties{nullptr};
            PFN_vkEnumerateInstanceVersion m_vkEnumerateInstanceVersion{nullptr};

            //
            // Instance calls function pointers
            //
            PFN_vkCreateDebugUtilsMessengerEXT m_vkCreateDebugUtilsMessengerEXT{nullptr};
            PFN_vkDestroyDebugUtilsMessengerEXT m_vkDestroyDebugUtilsMessengerEXT{nullptr};
            PFN_vkCmdBeginDebugUtilsLabelEXT m_vkCmdBeginDebugUtilsLabelEXT{nullptr};
            PFN_vkCmdEndDebugUtilsLabelEXT m_vkCmdEndDebugUtilsLabelEXT{nullptr};
            PFN_vkCmdInsertDebugUtilsLabelEXT m_vkCmdInsertDebugUtilsLabelEXT{nullptr};
            PFN_vkQueueBeginDebugUtilsLabelEXT m_vkQueueBeginDebugUtilsLabelEXT{nullptr};
            PFN_vkQueueEndDebugUtilsLabelEXT m_vkQueueEndDebugUtilsLabelEXT{nullptr};
            PFN_vkDestroyInstance m_vkDestroyInstance{nullptr};
            PFN_vkEnumeratePhysicalDevices m_vkEnumeratePhysicalDevices{nullptr};
            PFN_vkGetPhysicalDeviceProperties m_vkGetPhysicalDeviceProperties{nullptr};
            PFN_vkGetPhysicalDeviceFeatures m_vkGetPhysicalDeviceFeatures{nullptr};
            PFN_vkGetPhysicalDeviceFeatures2 m_vkGetPhysicalDeviceFeatures2{nullptr};
            PFN_vkGetPhysicalDeviceQueueFamilyProperties m_vkGetPhysicalDeviceQueueFamilyProperties{nullptr};
            PFN_vkCreateDevice m_vkCreateDevice{nullptr};
            PFN_vkDestroySurfaceKHR m_vkDestroySurfaceKHR{nullptr};
            PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_vkGetPhysicalDeviceSurfaceSupportKHR{nullptr};
            PFN_vkEnumerateDeviceExtensionProperties m_vkEnumerateDeviceExtensionProperties{nullptr};
            PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR{nullptr};
            PFN_vkGetPhysicalDeviceSurfaceFormatsKHR m_vkGetPhysicalDeviceSurfaceFormatsKHR{nullptr};
            PFN_vkGetPhysicalDeviceSurfacePresentModesKHR m_vkGetPhysicalDeviceSurfacePresentModesKHR{nullptr};
            PFN_vkGetPhysicalDeviceMemoryProperties m_vkGetPhysicalDeviceMemoryProperties{nullptr};
            PFN_vkGetPhysicalDeviceMemoryProperties2 m_vkGetPhysicalDeviceMemoryProperties2{nullptr};
            PFN_vkGetPhysicalDeviceFormatProperties m_vkGetPhysicalDeviceFormatProperties{nullptr};

            //
            // Device calls function pointers
            //
            PFN_vkSetDebugUtilsObjectNameEXT m_vkSetDebugUtilsObjectNameEXT{nullptr};
            PFN_vkDestroyDevice m_vkDestroyDevice{nullptr};
            PFN_vkGetDeviceQueue m_vkGetDeviceQueue{nullptr};
            PFN_vkCreateSwapchainKHR m_vkCreateSwapchainKHR{nullptr};
            PFN_vkDestroySwapchainKHR m_vkDestroySwapchainKHR{nullptr};
            PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHR{nullptr};
            PFN_vkCreateImageView m_vkCreateImageView{nullptr};
            PFN_vkDestroyImageView m_vkDestroyImageView{nullptr};
            PFN_vkCreateShaderModule m_vkCreateShaderModule{nullptr};
            PFN_vkDestroyShaderModule m_vkDestroyShaderModule{nullptr};
            PFN_vkCreatePipelineLayout m_vkCreatePipelineLayout{nullptr};
            PFN_vkDestroyPipelineLayout m_vkDestroyPipelineLayout{nullptr};
            PFN_vkCreateRenderPass m_vkCreateRenderPass{nullptr};
            PFN_vkDestroyRenderPass m_vkDestroyRenderPass{nullptr};
            PFN_vkCreateGraphicsPipelines m_vkCreateGraphicsPipelines{nullptr};
            PFN_vkDestroyPipeline m_vkDestroyPipeline{nullptr};
            PFN_vkCreateFramebuffer m_vkCreateFramebuffer{nullptr};
            PFN_vkDestroyFramebuffer m_vkDestroyFramebuffer{nullptr};
            PFN_vkCreateCommandPool m_vkCreateCommandPool{nullptr};
            PFN_vkDestroyCommandPool m_vkDestroyCommandPool{nullptr};
            PFN_vkAllocateCommandBuffers m_vkAllocateCommandBuffers{nullptr};
            PFN_vkFreeCommandBuffers m_vkFreeCommandBuffers{nullptr};
            PFN_vkBeginCommandBuffer m_vkBeginCommandBuffer{nullptr};
            PFN_vkCmdBeginRenderPass m_vkCmdBeginRenderPass{nullptr};
            PFN_vkCmdNextSubpass m_vkCmdNextSubpass{nullptr};
            PFN_vkCmdBindPipeline m_vkCmdBindPipeline{nullptr};
            PFN_vkCmdBindVertexBuffers m_vkCmdBindVertexBuffers{nullptr};
            PFN_vkCmdBindIndexBuffer m_vkCmdBindIndexBuffer{nullptr};
            PFN_vkCmdDraw m_vkCmdDraw{nullptr};
            PFN_vkCmdDrawIndexed m_vkCmdDrawIndexed{nullptr};
            PFN_vkCmdEndRenderPass m_vkCmdEndRenderPass{nullptr};
            PFN_vkEndCommandBuffer m_vkEndCommandBuffer{nullptr};
            PFN_vkCreateSemaphore m_vkCreateSemaphore{nullptr};
            PFN_vkDestroySemaphore m_vkDestroySemaphore{nullptr};
            PFN_vkAcquireNextImageKHR m_vkAcquireNextImageKHR{nullptr};
            PFN_vkQueueSubmit m_vkQueueSubmit{nullptr};
            PFN_vkQueuePresentKHR m_vkQueuePresentKHR{nullptr};
            PFN_vkQueueWaitIdle m_vkQueueWaitIdle{nullptr};
            PFN_vkDeviceWaitIdle m_vkDeviceWaitIdle{nullptr};
            PFN_vkResetCommandBuffer m_vkResetCommandBuffer{nullptr};
            PFN_vkResetCommandPool m_vkResetCommandPool{nullptr};
            PFN_vkCreateFence m_vkCreateFence{nullptr};
            PFN_vkWaitForFences m_vkWaitForFences{nullptr};
            PFN_vkResetFences m_vkResetFences{nullptr};
            PFN_vkDestroyFence m_vkDestroyFence{nullptr};
            PFN_vkAllocateMemory m_vkAllocateMemory{nullptr};
            PFN_vkFreeMemory m_vkFreeMemory{nullptr};
            PFN_vkMapMemory m_vkMapMemory{nullptr};
            PFN_vkUnmapMemory m_vkUnmapMemory{nullptr};
            PFN_vkFlushMappedMemoryRanges m_vkFlushMappedMemoryRanges{nullptr};
            PFN_vkInvalidateMappedMemoryRanges m_vkInvalidateMappedMemoryRanges{nullptr};
            PFN_vkBindBufferMemory m_vkBindBufferMemory{nullptr};
            PFN_vkBindImageMemory m_vkBindImageMemory{nullptr};
            PFN_vkGetBufferMemoryRequirements m_vkGetBufferMemoryRequirements{nullptr};
            PFN_vkGetImageMemoryRequirements m_vkGetImageMemoryRequirements{nullptr};
            PFN_vkCreateBuffer m_vkCreateBuffer{nullptr};
            PFN_vkDestroyBuffer m_vkDestroyBuffer{nullptr};
            PFN_vkCreateImage m_vkCreateImage{nullptr};
            PFN_vkDestroyImage m_vkDestroyImage{nullptr};
            PFN_vkCmdCopyBuffer m_vkCmdCopyBuffer{nullptr};
            PFN_vkGetBufferMemoryRequirements2 m_vkGetBufferMemoryRequirements2{nullptr};
            PFN_vkGetImageMemoryRequirements2 m_vkGetImageMemoryRequirements2{nullptr};
            PFN_vkBindBufferMemory2 m_vkBindBufferMemory2{nullptr};
            PFN_vkBindImageMemory2 m_vkBindImageMemory2{nullptr};
            PFN_vkCmdPushConstants m_vkCmdPushConstants{nullptr};
            PFN_vkCreateDescriptorPool m_vkCreateDescriptorPool{nullptr};
            PFN_vkDestroyDescriptorPool m_vkDestroyDescriptorPool{nullptr};
            PFN_vkCreateDescriptorSetLayout m_vkCreateDescriptorSetLayout{nullptr};
            PFN_vkDestroyDescriptorSetLayout m_vkDestroyDescriptorSetLayout{nullptr};
            PFN_vkAllocateDescriptorSets m_vkAllocateDescriptorSets{nullptr};
            PFN_vkUpdateDescriptorSets m_vkUpdateDescriptorSets{nullptr};
            PFN_vkCmdBindDescriptorSets m_vkCmdBindDescriptorSets{nullptr};
            PFN_vkCmdPipelineBarrier m_vkCmdPipelineBarrier{nullptr};
            PFN_vkCmdCopyBufferToImage m_vkCmdCopyBufferToImage{nullptr};
            PFN_vkResetDescriptorPool m_vkResetDescriptorPool{nullptr};
            PFN_vkCreateSampler m_vkCreateSampler{nullptr};
            PFN_vkDestroySampler m_vkDestroySampler{nullptr};
            PFN_vkGetFenceStatus m_vkGetFenceStatus{nullptr};
            PFN_vkFreeDescriptorSets m_vkFreeDescriptorSets{nullptr};
            PFN_vkCmdCopyImage m_vkCmdCopyImage{nullptr};
            PFN_vkCmdSetViewport m_vkCmdSetViewport{nullptr};
            PFN_vkCmdClearAttachments m_vkCmdClearAttachments{nullptr};
            PFN_vkCmdBlitImage m_vkCmdBlitImage{nullptr};
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VULKANCALLS_H
