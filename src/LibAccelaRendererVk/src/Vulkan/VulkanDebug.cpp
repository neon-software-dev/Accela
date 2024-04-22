/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDebug.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

#include <Accela/Common/BuildInfo.h>

namespace Accela::Render
{

//#define NO_VULKAN_DEBUG

void SetDebugName(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkObjectType objType, uint64_t obj, const std::string& name)
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = objType;
    nameInfo.objectHandle = obj;
    nameInfo.pObjectName = name.c_str();

    vk->vkSetDebugUtilsObjectNameEXT(device->GetVkDevice(), &nameInfo);
}

void RemoveDebugName(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkObjectType objType, uint64_t obj)
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = objType;
    nameInfo.objectHandle = obj;
    nameInfo.pObjectName = nullptr;

    vk->vkSetDebugUtilsObjectNameEXT(device->GetVkDevice(), &nameInfo);
}

CmdBufferSectionLabel::CmdBufferSectionLabel(IVulkanCallsPtr vk, const VulkanCommandBufferPtr& cmdBuffer, const std::string& sectionName)
    : m_vk(std::move(vk))
    , m_vkCmdBuffer(cmdBuffer->GetVkCommandBuffer())
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    VkDebugUtilsLabelEXT labelInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        sectionName.c_str(),
        {0.0f, 0.5f, 0.5f, 1.0f}
    };

    m_vk->vkCmdBeginDebugUtilsLabelEXT(m_vkCmdBuffer, &labelInfo);
}

CmdBufferSectionLabel::~CmdBufferSectionLabel()
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    m_vk->vkCmdEndDebugUtilsLabelEXT(m_vkCmdBuffer);
}

QueueSectionLabel::QueueSectionLabel(IVulkanCallsPtr vk, VkQueue vkQueue, const std::string& sectionName)
    : m_vk(std::move(vk))
    , m_vkQueue(vkQueue)
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    VkDebugUtilsLabelEXT labelInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        nullptr,
        sectionName.c_str(),
        {0.0f, 0.0f, 1.0f, 1.0f}
    };

    m_vk->vkQueueBeginDebugUtilsLabelEXT(m_vkQueue, &labelInfo);
}

QueueSectionLabel::~QueueSectionLabel()
{
    #ifdef NO_VULKAN_DEBUG
        return;
    #endif

    if (!Common::BuildInfo::IsDebugBuild())
    {
        return;
    }

    m_vk->vkQueueEndDebugUtilsLabelEXT(m_vkQueue);
}

}
