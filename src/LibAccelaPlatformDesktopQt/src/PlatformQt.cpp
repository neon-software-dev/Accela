#include <Accela/Platform/PlatformQt.h>
#include <Accela/Platform/Window/QtWindow.h>

#include "Event/QtEvents.h"

namespace Accela::Platform
{

PlatformQt::PlatformQt(Common::ILogger::Ptr logger)
    : PlatformDesktop(std::move(logger))
    , m_qtVulkanInstance(std::make_shared<QtVulkanInstance>(m_logger))
    , m_events(std::make_shared<QtEvents>(m_logger))
    , m_window(std::make_shared<QtWindow>(m_logger, m_qtVulkanInstance))
{

}

bool PlatformQt::Startup()
{
    if (!PlatformDesktop::Startup()) { return false; }

    m_logger->Log(Common::LogLevel::Info, "PlatformQt: Starting");

    if (!m_qtVulkanInstance->Init())
    {
        return false;
    }

    return true;
}

void PlatformQt::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "PlatformQt: Shutting down");

    m_qtVulkanInstance->Destroy();

    PlatformDesktop::Shutdown();
}

IEvents::Ptr PlatformQt::GetEvents() const noexcept { return m_events; }
IWindow::Ptr PlatformQt::GetWindow() const noexcept { return m_window; }
QtVulkanInstance::Ptr PlatformQt::GetQtVulkanInstance() const noexcept { return m_qtVulkanInstance; }

}
