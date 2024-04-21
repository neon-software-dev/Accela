/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINEDESKTOP_SRC_DESKTOPVULKANCALLS_H
#define LIBACCELAENGINEDESKTOP_SRC_DESKTOPVULKANCALLS_H

#include <Accela/Render/VulkanCalls.h>

#include <SDL2/SDL_vulkan.h>

namespace Accela::Engine
{
    /**
     * Uses SDL to retrieve the vkGetInstanceProcAddr function
     */
    class DesktopVulkanCalls : public Render::VulkanCalls
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

#endif //LIBACCELAENGINEDESKTOP_SRC_DESKTOPVULKANCALLS_H
