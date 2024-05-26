#include <Accela/Platform/QtVulkanInstance.h>

namespace Accela::Platform
{

QtVulkanInstance::QtVulkanInstance(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

bool QtVulkanInstance::Init()
{
    m_pVulkanInstance = new QVulkanInstance();

    if (!m_pVulkanInstance->create())
    {
        m_logger->Log(Common::LogLevel::Error, "QtVulkanInstance::Init: Failed to create instance");
        return false;
    }

    return true;
}

void QtVulkanInstance::Destroy()
{
    if (m_pVulkanInstance != nullptr)
    {
        m_pVulkanInstance->destroy();
        m_pVulkanInstance = nullptr;
    }
}

bool QtVulkanInstance::CreateFromVkInstance(VkInstance vkInstance)
{
    if (m_pVulkanInstance != nullptr)
    {
        m_pVulkanInstance->destroy();
    }

    m_pVulkanInstance = new QVulkanInstance();
    m_pVulkanInstance->setVkInstance(vkInstance);

    if (!m_pVulkanInstance->create())
    {
        m_logger->Log(Common::LogLevel::Error,
          "QtVulkanInstance::CreateFromVkInstance: Failed to create instance");
        return false;
    }

    return true;
}

QVulkanInstance *QtVulkanInstance::GetQVulkanInstance() const noexcept
{
    return m_pVulkanInstance;
}

}
