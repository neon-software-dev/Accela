/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_POSTEXECUTIONOPS_H
#define LIBACCELARENDERERVK_SRC_POSTEXECUTIONOPS_H

#include "ForwardDeclares.h"

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <stack>
#include <unordered_map>
#include <functional>

namespace Accela::Render
{
    using PostExecutionOp = std::function<void()>;

    /**
     * Enqueues operations to be executed after both all in-progress frame
     * work and the operation's own queue work has finished executing.
     */
    class PostExecutionOps
    {
        public:

            PostExecutionOps(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs);

            bool Initialize(const RenderSettings& renderSettings);
            bool OnRenderSettingsChanged(const RenderSettings& renderSettings);
            void Destroy();

            // Enqueue the provided operation to run when the provided fence has finished and when
            // all frames have finished their render work one time since this method was called
            void Enqueue(VkFence vkFence, const PostExecutionOp& op);

            // Enqueue the provided operation to run when the provided fence has finished, without
            // waiting for any render work to finish in addition
            void EnqueueFrameless(VkFence vkFence, const PostExecutionOp& op);

            // Same as Enqueue(..), except the fence being waited on is the current frame's work fence
            void Enqueue_Current(const PostExecutionOp& op);

            // Reports that provided frame/fence has finished its work, and starts the
            // fulfill process for any pending operations that are ready to run
            void SetFrameSynced(uint8_t frameIndex, VkFence vkFence);

            // Starts the fulfill process for any pending operations that are ready to run
            void FulfillReady();

            // Blocking. Waits for all GPU work to finish then forces all pending operations to run
            void FulfillAll();

        private:

            struct ExecutionData
            {
                explicit ExecutionData(uint8_t framesInFlight)
                    : framesFinished(std::vector<bool>(framesInFlight, false))
                { }

                // Tracks, for a particular fence, which frames have finished executing
                // since the moment the operations for the fence were enqueued.
                std::vector<bool> framesFinished;

                // Operations that should be executed when the fence has finished & all frames rendered
                std::stack<PostExecutionOp> frameOps;

                // Operations that should be executed when only the fence has finished
                std::stack<PostExecutionOp> framelessOps;
            };

        private:

            void FulfillReadyInternal(bool forceReady);

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;

            uint8_t m_framesInFlight{0};

            // Note: VkFence can be VK_NULL_HANDLE for ops that are enqueued before
            // any frames have been rendered
            std::unordered_map<VkFence, ExecutionData> m_data;

            VkFence m_currentFrameFence{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_POSTEXECUTIONOPS_H
