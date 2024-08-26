/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_IVULKANCONTEXT_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_IVULKANCONTEXT_H

#include <Accela/Render/Eye.h>

#include <Accela/Common/SharedLib.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <set>
#include <string>
#include <optional>

namespace Accela::Render
{
    struct ACCELA_PUBLIC HeadsetEyeRenderData
    {
        VkInstance vkInstance;
        VkPhysicalDevice vkPhysicalDevice;
        VkDevice vkDevice;
        VkQueue vkQueue;
        VkImage vkImage;
        uint32_t queueFamilyIndex;
        uint32_t width;
        uint32_t height;
        uint32_t format;
        uint32_t sampleCount;
    };

    /**
     * Interface for the renderer to interact with the active Vulkan context
     */
    class ACCELA_PUBLIC IVulkanContext
    {
        public:

            virtual ~IVulkanContext() = default;

            /**
             * Fetch the Vulkan instance extensions that the system requires be available
             *
             * @param extensions Receives the result
             *
             * @return Whether the required extensions could be fetched successfully
             */
            [[nodiscard]] virtual bool GetRequiredInstanceExtensions(std::set<std::string>& extensions) const = 0;

            /**
             * Fetch the Vulkan device extensions that the system requires be available
             *
             * @param vkPhysicalDevice The physical device in question
             * @param extensions Receives the result
             *
             * @return Whether the required extensions could be fetched successfully
             */
            [[nodiscard]] virtual bool GetRequiredDeviceExtensions(VkPhysicalDevice vkPhysicalDevice, std::set<std::string>& extensions) const = 0;

            /**
             * Create a Vulkan surface for the renderer to use.
             *
             * @param instance The Vulkan Instance that will be used
             * @param pSurface Receives the result
             *
             * @return Whether the surface could be created successfully
             */
            [[nodiscard]] virtual bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* pSurface) const = 0;

            /**
             * Retrieve the pixel size of the current Vulkan surface
             *
             * @param size Receives the result
             *
             * @return Whether the result could be set, or false if no vulkan surface exists
             */
            [[nodiscard]] virtual bool GetSurfacePixelSize(std::pair<unsigned int, unsigned int>& size) const  = 0;
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_IVULKANCONTEXT_H
