/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanSurface.h"
#include "VulkanInstance.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>

namespace Accela::Render
{

VulkanSurface::VulkanSurface(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_vulkanContext(std::move(vulkanContext))
{

}

bool VulkanSurface::Create(const VulkanInstancePtr& instance) noexcept
{
    if (!m_vulkanContext->CreateVulkanSurface(instance->GetVkInstance(), &m_vkSurface))
    {
        m_logger->Log(Common::LogLevel::Fatal, "VulkanSurface: Call to create a surface failed");
        return false;
    }

    m_instance = instance;

    return true;
}

void VulkanSurface::Destroy() noexcept
{
    if (m_vkSurface == VK_NULL_HANDLE)
    {
        return;
    }

    m_vulkanCalls->vkDestroySurfaceKHR(m_instance->GetVkInstance(), m_vkSurface, nullptr);
    m_vkSurface = VK_NULL_HANDLE;

    m_instance = nullptr;
}

std::pair<unsigned int, unsigned int> VulkanSurface::GetSurfaceSize() const
{
    std::pair<unsigned int, unsigned int> result;
    (void)m_vulkanContext->GetSurfacePixelSize(result);

    return result;
}

}
