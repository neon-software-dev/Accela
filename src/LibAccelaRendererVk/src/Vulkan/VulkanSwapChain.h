#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANSWAPCHAIN
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANSWAPCHAIN

#include "SwapChainSupportDetails.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <memory>
#include <vector>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with swap chains
     */
    class VulkanSwapChain
    {
        public:

            /**
             * Contains details about the swap chain
             */
            struct SwapChainConfig
            {
                SwapChainConfig(VkSurfaceFormatKHR _surfaceFormat,
                                VkPresentModeKHR _presentMode,
                                VkExtent2D _extent,
                                VkSurfaceTransformFlagBitsKHR _preTransform)
                    : surfaceFormat(_surfaceFormat)
                    , presentMode(_presentMode)
                    , extent(_extent)
                    , preTransform(_preTransform)
                { }

                VkSurfaceFormatKHR surfaceFormat;           // The format of the swap chain images
                VkPresentModeKHR presentMode;               // The current present mode
                VkExtent2D extent;                          // The extent of the swap chain images
                VkSurfaceTransformFlagBitsKHR preTransform; // Surface pre-transform settings
            };

        public:

            VulkanSwapChain(Common::ILogger::Ptr logger,
                            IVulkanCallsPtr vk,
                            IVMAPtr vma,
                            VulkanPhysicalDevicePtr physicalDevice,
                            VulkanDevicePtr device);

            /**
             * Create this swap chain
             *
             * @param surface The surface that will be presented to
             * @param previousSwapChain A previous swap chain that's being replaced, if any
             * @param desiredPresentMode The desired present mode. May not be used if not supported.
             *
             * @return Whether the swap chain was created successfully
             */
            bool Create(const VulkanSurfacePtr& surface,
                        const VulkanSwapChainPtr& previousSwapChain,
                        const PresentMode& desiredPresentMode);

            /**
             * Destroy this swap chain
             */
            void Destroy();

            /**
             * @return The configuration details of the current swap chain
             */
            [[nodiscard]] std::optional<SwapChainConfig> GetConfig() const { return m_swapChainConfig; }

            /**
             * @return The VkSwapchainKHR object associated with this swap chain
             */
            [[nodiscard]] VkSwapchainKHR GetVkSwapchainKHR() const { return m_vkSwapChain; }

            /**
             * @return The VkImages this swap chain refers to
             */
            [[nodiscard]] std::vector<VkImage> GetSwapChainImages() const { return m_swapChainImages; }

            /**
             * @return The surface ImageViews this swap chain uses for presenting
             */
            [[nodiscard]] std::vector<VkImageView> GetSwapChainImageViews() const { return m_swapChainImageViews; }

        private:

            static VkSurfaceFormatKHR ChooseSurfaceFormat(const SwapChainSupportDetails& supportDetails);
            VkPresentModeKHR ChoosePresentMode(const SwapChainSupportDetails& supportDetails, const PresentMode& desiredPresentMode);
            static VkExtent2D ChooseExtent(const VulkanSurfacePtr& surface, const SwapChainSupportDetails& supportDetails);

            static bool HasSupportForPresentMode(const SwapChainSupportDetails& supportDetails, VkPresentModeKHR presentMode);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            IVMAPtr m_vma;
            VulkanPhysicalDevicePtr m_physicalDevice;
            VulkanDevicePtr m_device;

            VkSwapchainKHR m_vkSwapChain{VK_NULL_HANDLE};
            std::optional<SwapChainConfig> m_swapChainConfig;

            std::vector<VkImage> m_swapChainImages;
            std::vector<VkImageView> m_swapChainImageViews;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANSWAPCHAIN
