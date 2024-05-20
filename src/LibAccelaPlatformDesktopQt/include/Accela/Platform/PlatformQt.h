/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
#define LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H

#include "QtVulkanInstance.h"

#include <Accela/Platform/PlatformDesktop.h>

namespace Accela::Platform
{
    /**
     * Qt-based implementation of IPlatform system for use on desktop (Linux and Windows) environments.
     */
    class PlatformQt : public PlatformDesktop
    {
        public:

            using Ptr = std::shared_ptr<PlatformQt>;

        public:

            explicit PlatformQt(Common::ILogger::Ptr logger);

            bool Startup() override;
            void Shutdown() override;

            [[nodiscard]] IEvents::Ptr GetEvents() const noexcept override;
            [[nodiscard]] IWindow::Ptr GetWindow() const noexcept override;

            [[nodiscard]] QtVulkanInstance::Ptr GetQtVulkanInstance() const noexcept;

        private:

            QtVulkanInstance::Ptr m_qtVulkanInstance;

            IEvents::Ptr m_events;
            IWindow::Ptr m_window;
    };
}

#endif //LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
