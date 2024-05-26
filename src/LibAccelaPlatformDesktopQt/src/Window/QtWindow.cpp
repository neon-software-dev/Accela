/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/Window/QtWindow.h>

#include <QVulkanInstance>
#include <QVulkanWindow>

namespace Accela::Platform
{

QtWindow::QtWindow(Common::ILogger::Ptr logger, QtVulkanInstance::Ptr qtVulkanInstance)
    : m_logger(std::move(logger))
    , m_qtVulkanInstance(std::move(qtVulkanInstance))
{

}

void QtWindow::AttachToWindow(QWindow* pWindow)
{
    m_logger->Log(Common::LogLevel::Debug,
      "QtWindow: Attached to window: {}", (std::size_t)pWindow);

    m_pWindow = pWindow;
}

std::expected<std::pair<unsigned int, unsigned int>, bool> QtWindow::GetWindowSize() const
{
    assert(m_pWindow != nullptr);

    return std::make_pair(m_pWindow->size().width(), m_pWindow->size().height());
}

std::expected<std::pair<unsigned int, unsigned int>, bool> QtWindow::GetWindowDisplaySize() const
{
    return std::unexpected(false);
}

bool QtWindow::LockCursorToWindow(bool lock) const
{
    (void)lock;
    return true;
}

bool QtWindow::SetFullscreen(bool fullscreen) const
{
    (void)fullscreen;
    return true;
}

bool QtWindow::SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const
{
    (void)size;
    return true;
}

bool QtWindow::GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const
{
    //
    // Query for all the extensions the default QVulkanInstance enables, and pass them all on
    // as extensions that the renderer should also enable when it creates its instance
    //
    const auto& defaultInstanceExtensions = m_qtVulkanInstance->GetQVulkanInstance()->extensions();
    for (unsigned int x = 0; x < defaultInstanceExtensions.count(); ++x)
    {
        const auto& defaultExtension = defaultInstanceExtensions.at(x);
        extensions.push_back(defaultExtension.toStdString());

        m_logger->Log(Common::LogLevel::Info, "Qt Default Extension: {}", defaultExtension.toStdString());
    }

    return true;
}

bool QtWindow::CreateVulkanSurface(VkInstance, VkSurfaceKHR *pSurface) const
{
    assert(m_pWindow != nullptr);

    // VkInstance is passed in, but when the renderer was initializing its instance-based calls,
    // our QtVulkanInstance created a QVulkanInstance from it at that point

    // Now that we have a renderer instance created, and its asked us to create a surface, update
    // our window with the instance info, and then create a surface from the window
    m_pWindow->setVulkanInstance(m_qtVulkanInstance->GetQVulkanInstance());
    *pSurface = QVulkanInstance::surfaceForWindow(m_pWindow);

    return true;
}

}
