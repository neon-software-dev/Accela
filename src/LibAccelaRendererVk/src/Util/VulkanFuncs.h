/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_VULKANFUNCS_H
#define LIBACCELARENDERERVK_SRC_UTIL_VULKANFUNCS_H

#include "../ForwardDeclares.h"

#include "Synchronization.h"
#include "ImageAllocation.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/ImageData.h>
#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <utility>

namespace Accela::Render
{
    class VulkanFuncs
    {
        public:

            VulkanFuncs(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs);

            static std::optional<VkFormat> ImageDataFormatToVkFormat(const Common::ImageData::PixelFormat& format);

            [[nodiscard]] VkFormatProperties GetVkFormatProperties(const VkFormat& vkFormat) const;

            /**
             * Creates a command buffer, records the provided func into it, and executes the command
             * buffer on the provided queue.
             *
             * @param tag Debug tag to associate with the operation
             * @param vkQueue The queue for commands to be executed on
             * @param commandPool The command pool to allocate a command buffer from
             * @param func The func which records commands into a command buffer
             *
             * @return Whether the work was submitted successfully
             */
            bool QueueSubmit(
                const std::string& tag,
                const PostExecutionOpsPtr& postExecutionOps,
                VkQueue vkQueue,
                const VulkanCommandPoolPtr& commandPool,
                const std::function<void(const VulkanCommandBufferPtr&, VkFence)>& func
            );

            /**
             * Executes a command buffer on the provided queue
             *
             * @param tag Debug tag to associate with the operation
             * @param vkQueue The queue for the work to be submitted to
             * @param commandBuffers The command buffers to submit
             * @param waitOn Semaphores the work should wait on
             * @param signalOn Semaphores the work should signal
             * @param fence An optional fence to signal when the work is finished
             *
             * @return Whether the submit was successful
             */
            bool QueueSubmit(
                const std::string& tag,
                const VkQueue& vkQueue,
                const std::vector<VkCommandBuffer>& commandBuffers,
                const WaitOn& waitOn,
                const SignalOn& signalOn,
                const std::optional<VkFence>& fence
            );

            /**
             * Transfers image data (asynchronously) to (the base mip level of) a GPU VkImage
             *
             * @param buffers Buffers instance
             * @param postExecutionOps PostExecutionOps instance
             * @param vkCommandBuffer Command buffer to execute the commands on
             * @param vkExecutionFence Fence which tracks execution of vkCommandBuffer
             * @param imageData The image to be transferred
             * @param vkDestImage The VkImage to transfer to
             * @param mipLevels How many mipLevels the image has in total
             * @param vkPipelineUsageFlags Earliest pipeline stage the VkImage will be used in
             * @param vkFinalImageLayout The layout (each mip level) of the image should be set to
             * after the transfer is completed
             *
             * @return True if the transfer was started successfully
             */
            bool TransferImageData(
                const IBuffersPtr& buffers,
                const PostExecutionOpsPtr& postExecutionOps,
                VkCommandBuffer vkCommandBuffer,
                VkFence vkExecutionFence,
                const Common::ImageData::Ptr& imageData,
                VkImage vkDestImage,
                const uint32_t & mipLevels,
                VkPipelineStageFlags vkPipelineUsageFlags,
                VkImageLayout vkFinalImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            /**
             * Generates MipMaps for the specified vkImage. Requires the first mipmap level of the image
             * to already be filled out with the image's data.
             *
             * @param vkCommandBuffer Command buffer to execute the commands on
             * @param imageSize The width/height size of the base level mip map
             * @param vkImage The VkImage to have mipmaps generated for
             * @param mipLevels The number of mip maps the image has
             * @param vkPipelineUsageFlags Earliest pipeline stage the VkImage will be used in
             * @param vkFinalImageLayout The layout (each mip level) of the image should be set to
             * after the generation is completed
             */
            void GenerateMipMaps(
                VkCommandBuffer vkCommandBuffer,
                const USize& imageSize,
                VkImage vkImage,
                uint32_t mipLevels,
                VkPipelineStageFlags vkPipelineUsageFlags,
                VkImageLayout vkFinalImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_VULKANFUNCS_H
