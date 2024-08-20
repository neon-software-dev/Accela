/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_VULKANFUNCS_H
#define LIBACCELARENDERERVK_SRC_UTIL_VULKANFUNCS_H

#include "../ForwardDeclares.h"

#include "Synchronization.h"
#include "ImageAllocation.h"
#include "PostExecutionOps.h"

#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Texture/Texture.h>

#include <Accela/Common/ImageData.h>
#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <utility>
#include <future>

namespace Accela::Render
{
    class VulkanFuncs
    {
        public:

            VulkanFuncs(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs);

            [[nodiscard]] VkFormatProperties GetVkFormatProperties(const VkFormat& vkFormat) const;

            /**
             * Provides the ability to schedule one-off work to be performed on a VkQueue. If a post
             * execution func is supplied, it will also execute that function when the scheduled work
             * has finished executing (as configured by postExecutionEnqueueType param).
             *
             * Allocates a command buffer, records the provided func's commands into it, and executes
             * the command buffer on the provided queue.
             *
             * @param tag Debug tag to associate with the operation
             * @param postExecutionOps PostExecutionOps instance
             * @param vkQueue The queue for commands to be executed on
             * @param commandPool The command pool to allocate a command buffer from
             * @param func A func which records commands into a command buffer to be executed
             * @param postExecutionFunc An optional func which is called when the work scheduled by func has finished
             * @param postExecutionEnqueueType Specifies how to enqueue postExecutionFunc for execution
             *
             * @return Whether the work was submitted successfully
             */
            bool QueueSubmit(
                const std::string& tag,
                const PostExecutionOpsPtr& postExecutionOps,
                VkQueue vkQueue,
                const VulkanCommandPoolPtr& commandPool,
                const std::function<bool(const VulkanCommandBufferPtr&, VkFence)>& func,
                const std::optional<std::function<void(bool)>>& postExecutionFunc = std::nullopt,
                const EnqueueType& postExecutionEnqueueType = EnqueueType::Frame
            );

            /**
             * The same as the above QueueSubmit, except this one additionally takes in a promise which gets
             * its value set to the value that postExecutionFunc function returns when it's executed.
             */
            template <typename T>
            bool QueueSubmit(
                const std::string& tag,
                const PostExecutionOpsPtr& postExecutionOps,
                VkQueue vkQueue,
                const VulkanCommandPoolPtr& commandPool,
                const std::function<bool(const VulkanCommandBufferPtr&, VkFence)>& func,
                const std::function<T(bool)>& postExecutionFunc,
                std::promise<T> resultPromise,
                const EnqueueType& postExecutionEnqueueType = EnqueueType::Frame
            )
            {
                // Move the promise into a shared_ptr for the lambda to access
                const auto resultPromisePtr = std::make_shared<std::promise<T>>(std::move(resultPromise));

                // Submit the queue work but with a wrapper PostExecutionFunc which set the promise's
                // value to the result returned by postExecutionFunc
                return QueueSubmit(tag, postExecutionOps, vkQueue, commandPool, func,
                    [=](bool funcResult) {
                        resultPromisePtr->set_value(std::invoke(postExecutionFunc, funcResult));
                    },
                    postExecutionEnqueueType
                );
            }

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
