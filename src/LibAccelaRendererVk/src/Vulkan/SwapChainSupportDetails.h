#ifndef LIBACCELARENDERERVK_SRC_VULKAN_SWAPCHAINSUPPORTDETAILS
#define LIBACCELARENDERERVK_SRC_VULKAN_SWAPCHAINSUPPORTDETAILS

#include "../ForwardDeclares.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Accela::Render
{
    struct SwapChainSupportDetails
    {
            static SwapChainSupportDetails Load(const IVulkanCallsPtr& vulkanCalls,
                                                const VkPhysicalDevice& physicalDevice,
                                                const VkSurfaceKHR& surface);

            VkSurfaceCapabilitiesKHR capabilities{};
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;

        private:

            SwapChainSupportDetails() = default;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_SWAPCHAINSUPPORTDETAILS
