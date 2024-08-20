/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/QtVulkanCalls.h>

namespace Accela::Platform
{

QtVulkanCalls::QtVulkanCalls(QtVulkanInstance::Ptr qtVulkanInstance)
    : m_qtVulkanInstance(std::move(qtVulkanInstance))
{

}

bool QtVulkanCalls::InitInstanceCalls(VkInstance vkInstance)
{
    //
    // Up until the Renderer create an instance, we were using a default
    // QVulkanInstance. Now that we're at the point where we're looking
    // up functions for a particular instance, notify QtVulkanInstance to
    // recreate its internal QVulkanInstance based on the VkInstance that
    // the renderer is providing.
    //
    if (!m_qtVulkanInstance->CreateFromVkInstance(vkInstance))
    {
        return false;
    }

    // Technically not needed, since global funcs are never called again
    // after instance creation
    if (!Render::VulkanCalls::InitGlobalCalls())
    {
        return false;
    }

    //
    // Continue with the normal instance calls lookup using the new QVulkanInstance
    //
    return Render::VulkanCalls::InitInstanceCalls(vkInstance);
}

PFN_vkGetInstanceProcAddr QtVulkanCalls::GetInstanceProcAddrFunc()
{
    return (PFN_vkGetInstanceProcAddr)m_qtVulkanInstance->GetQVulkanInstance()
    ->getInstanceProcAddr("vkGetInstanceProcAddr");
}

}
