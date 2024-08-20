/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H

#include "QtVulkanInstance.h"

#include <Accela/Render/VulkanCalls.h>

#include <Accela/Common/SharedLib.h>

#include <QVulkanInstance>

namespace Accela::Platform
{
    /**
     * Uses SDL to retrieve the vkGetInstanceProcAddr function
     */
    class ACCELA_PUBLIC QtVulkanCalls : public Render::VulkanCalls
    {
        public:

            explicit QtVulkanCalls(QtVulkanInstance::Ptr qtVulkanInstance);

            bool InitInstanceCalls(VkInstance vkInstance) override;

        protected:

            PFN_vkGetInstanceProcAddr GetInstanceProcAddrFunc() override;

        private:

            QtVulkanInstance::Ptr m_qtVulkanInstance;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H
