#ifndef LIBACCELARENDERERVK_SRC_UTIL_EXECUTIONCONTEXT_H
#define LIBACCELARENDERERVK_SRC_UTIL_EXECUTIONCONTEXT_H

#include "../ForwardDeclares.h"

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    struct ExecutionContext
    {
        public:

            enum class Type
            {
                CPU,
                GPU
            };

            static ExecutionContext CPU()
            {
                return {};
            }

            static ExecutionContext GPU(VulkanCommandBufferPtr commandBuffer, VkFence vkFence)
            {
                return {std::move(commandBuffer), vkFence};
            }

        private:

            ExecutionContext()
                : type(Type::CPU)
            { }

            ExecutionContext(VulkanCommandBufferPtr _commandBuffer, VkFence _vkFence)
                : type(Type::GPU)
                , commandBuffer(std::move(_commandBuffer))
                , vkFence(_vkFence)
            { }

        public:

            Type type;
            VulkanCommandBufferPtr commandBuffer;
            VkFence vkFence{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_EXECUTIONCONTEXT_H
