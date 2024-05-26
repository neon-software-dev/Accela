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

    enum class EnqueueType
    {
        Frame,      // Waits on both a fence and a round of frame renders to finish
        Frameless   // Waits on only a fence to finish
    };

    /**
     * Enqueues operations to be executed after fence-based work and/or a round of frame
     * renders have finished.
     */
    class PostExecutionOps
    {
        public:

            PostExecutionOps(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs);

            bool Initialize(const RenderSettings& renderSettings);
            bool OnRenderSettingsChanged(const RenderSettings& renderSettings);
            void Destroy();

            // Enqueue an operation to be executed when the provided fence has signaled completion,
            // and when all frames have finished rendering at least one time since the operation
            // was enqueued
            void Enqueue(VkFence vkFence, const PostExecutionOp& op);

            // Enqueue an operation to be executed when the provided fence has signaled completion
            void EnqueueFrameless(VkFence vkFence, const PostExecutionOp& op);

            // Same as Enqueue, except the fence being waited on is the current frame's work fence
            void Enqueue_Current(const PostExecutionOp& op);

            // Reports that provided frame/fence has finished its work, and starts the
            // fulfill process for any pending operations that are ready to run
            void SetFrameSynced(uint8_t frameIndex, VkFence vkFence);

            // Starts the fulfill process for any pending operations that are ready to run
            void FulfillReady();

            // Blocking. Waits for all GPU work to finish then forces all pending operations to run
            void FulfillAll();

        private:

            void Enqueue(EnqueueType enqueueType, VkFence vkFence, const PostExecutionOp& op);

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
