/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPSDL_INCLUDE_ACCELA_PLATFORM_SDLVULKANCALLS_H
#define LIBACCELAPLATFORMDESKTOPSDL_INCLUDE_ACCELA_PLATFORM_SDLVULKANCALLS_H

#include <Accela/Render/VulkanCalls.h>

#include <Accela/Common/SharedLib.h>

#include <SDL2/SDL_vulkan.h>

namespace Accela::Platform
{
    /**
     * Uses SDL to retrieve the vkGetInstanceProcAddr function
     */
    class ACCELA_PUBLIC SDLVulkanCalls : public Render::VulkanCalls
    {
        protected:

            PFN_vkGetInstanceProcAddr GetInstanceProcAddrFunc() override
            {
                if (SDL_Vulkan_LoadLibrary(nullptr) != 0)
                {
                    return nullptr;
                }

                return (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
            }
    };
}

#endif //LIBACCELAPLATFORMDESKTOPSDL_INCLUDE_ACCELA_PLATFORM_SDLVULKANCALLS_H
