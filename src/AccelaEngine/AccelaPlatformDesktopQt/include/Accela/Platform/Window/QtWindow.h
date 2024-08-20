/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_SRC_WINDOW_QTWINDOW_H
#define LIBACCELAPLATFORMDESKTOPQT_SRC_WINDOW_QTWINDOW_H

#include "../QtVulkanInstance.h"

#include <Accela/Platform/Window/IWindow.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>

class QWindow;

namespace Accela::Platform
{
    /**
     * Qt-powered window functionality
     */
    class ACCELA_PUBLIC QtWindow : public IWindow
    {
        public:

            QtWindow(Common::ILogger::Ptr logger, QtVulkanInstance::Ptr qtVulkanInstance);

            // Set the QWindow that IWindow calls query and manipulate
            void AttachToWindow(QWindow* qWindow);

            //
            // IWindow
            //
            [[nodiscard]] std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowSize() const override;
            [[nodiscard]] std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowDisplaySize() const override;
            [[nodiscard]] bool LockCursorToWindow(bool lock) const override;
            [[nodiscard]] bool SetFullscreen(bool fullscreen) const override;
            [[nodiscard]] bool SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const override;
            [[nodiscard]] bool GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const override;
            [[nodiscard]] bool CreateVulkanSurface(void* pVkInstance, void* pVkSurface) const override;

        private:

            Common::ILogger::Ptr m_logger;
            QtVulkanInstance::Ptr m_qtVulkanInstance;

            QWindow* m_pWindow{nullptr};
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_SRC_WINDOW_QTWINDOW_H
