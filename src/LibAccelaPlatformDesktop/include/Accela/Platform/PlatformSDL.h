/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
#define LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H

#include <Accela/Platform/IPlatform.h>

#include <Accela/Common/Log/ILogger.h>

#include <string>
#include <vector>

namespace Accela::Platform
{
    /**
     * SDL-based implementation of IPlatform system for use on desktop (Linux and Windows) environments.
     */
    class PlatformSDL : public IPlatform
    {
        public:

            using Ptr = std::shared_ptr<PlatformSDL>;

        public:

            explicit PlatformSDL(Common::ILogger::Ptr logger);

            bool Startup() noexcept;
            void Shutdown() noexcept;

            [[nodiscard]] IEvents::Ptr GetEvents() const noexcept override;
            [[nodiscard]] IFiles::Ptr GetFiles() const noexcept override;
            [[nodiscard]] IText::Ptr GetText() const noexcept override;
            [[nodiscard]] IWindow::Ptr GetWindow() const noexcept override;
            [[nodiscard]] IVR::Ptr GetVR() const noexcept override;

        private:

            Common::ILogger::Ptr m_logger;

            IEvents::Ptr m_events;
            IFiles::Ptr m_files;
            IText::Ptr m_text;
            IWindow::Ptr m_window;
            IVR::Ptr m_vr;
    };
}

#endif //LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
