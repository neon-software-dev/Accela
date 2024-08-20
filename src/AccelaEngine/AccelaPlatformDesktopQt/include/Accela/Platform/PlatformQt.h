/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H
#define LIBACCELAPLATFORMSDL_INCLUDE_ACCELA_PLATFORM_PLATFORMSDL_H

#include "QtVulkanInstance.h"

#include <Accela/Platform/PlatformDesktop.h>

#include <Accela/Common/SharedLib.h>

namespace Accela::Platform
{
    /**
     * Qt-based implementation of IPlatform system for use on desktop (Linux and Windows) environments.
     *
     * Platform Notes:
     *
     * [Input Handling]
     *
     * Qt is incredibly limited with regards to input handling compared to SDL. There's a number of limitations
     * affecting the client when using a Qt-based platform system. Educate yourself on physical vs logical keys,
     * scancodes vs virtual codes before reading the following:
     *
     * 1) Qt does not provide a cross-platform physical scancode definition. PhysicalKeyPair::key will always be set to
     * PhysicalKey::Unknown and PhysicalKeyPair::scanCode value will always be set to an OS-specific scancode.
     *
     * 2) LogicalKeyPair::key will always be set to a value for supported keys for English keyboards, and set to Unknown
     * otherwise. LogicalKeyPair::virtualCode will always be set to an an OS-specific virtual keycode.
     *
     * 3) Qt provides no way to query for actively pressed physical or logical keys, other than for logical modifier keys.
     * That means that IKeyboardState::IsPhysicalKeyPressed(..) functionality is limited. The PhysicalKey argument
     * version will always fail (see item 1, we can't know what physical keys are pressed), but the ScanCode argument
     * version will still work correctly. One current bug/limitation of this system is that if you press a key, tab to
     * another OS window, and release the key, then when returning to the engine window IsPhysicalKeyPressed will still
     * say the key is pressed, until the next time the key is toggled. You may call IKeyboardState::ForceResetState to
     * clear out this erroneous state, such as when your window is re-focused. That all being said, if you specifically
     * need to test for whether a modifier (shift/control) logical key is actively pressed, then using
     * IKeyboardState::IsModifierPressed will work in that case, without any limitations.
     *
     * [Events]
     *
     * The client must install an Application-level and Widget-level event filter, and pass Qt events to
     * QtEvents::OnLocalEvent/OnGlobalEvent. Qt events delivered specifically to the Accela widget go to OnLocalEvent,
     * Qt events delivered globally to the application go to OnGlobalEvent. The division allows for supporting
     * multi-widget use cases: the engine is able to ignore some events when it is not focused, but still listen to
     * global events to do things such as build a global mapping of what keyboard keys are pressed, irregardless of
     * whether the engine is the widget with active keyboard focus.
     *
     */
    class ACCELA_PUBLIC PlatformQt : public PlatformDesktop
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
