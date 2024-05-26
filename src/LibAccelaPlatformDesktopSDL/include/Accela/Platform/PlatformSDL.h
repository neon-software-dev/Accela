#ifndef LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
#define LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H

#include <Accela/Platform/PlatformDesktop.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Platform
{
    /**
     * SDL-based implementation of IPlatform system for use on desktop (Linux and Windows) environments.
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
