/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
#define LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H

#include <Accela/Platform/PlatformDesktop.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Platform
{
    /**
     * SDL-based implementation of IPlatform system for use on desktop (Linux and Windows) environments.
     *
     * Platform Notes:
     *
     * [Input Handling]
     *
     * 1) PhysicalKeyPair::key will always be set to a value for supported keys for English keyboards, and set to Unknown
     * otherwise. PhysicalKeyPair::scanCode will always be set to an SDL-specific scancode value (SDL_SCANCODE_{X}).
     *
     * 2) LogicalKeyPair::key will always be set to a value for supported keys for English keyboards, and set to Unknown
     * otherwise. LogicalKeyPair::virtualCode will always be set to an SDL-specific virtual key code (SDLK_{x})
     */
    class PlatformSDL : public PlatformDesktop
    {
        public:

            using Ptr = std::shared_ptr<PlatformSDL>;

        public:

            explicit PlatformSDL(Common::ILogger::Ptr logger);

            [[nodiscard]] bool Startup() override;
            void Shutdown() override;

            [[nodiscard]] IEvents::Ptr GetEvents() const noexcept override;
            [[nodiscard]] IWindow::Ptr GetWindow() const noexcept override;

        private:


            IEvents::Ptr m_events;
            IWindow::Ptr m_window;
    };
}

#endif //LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
