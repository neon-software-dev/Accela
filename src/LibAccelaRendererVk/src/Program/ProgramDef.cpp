/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ProgramDef.h"

#include "../Util/DescriptorSets.h"

#include <format>
#include <algorithm>

namespace Accela::Render
{

ProgramDef::ProgramDef(std::string programName,
                       std::vector<std::string> shaderNames,
                       std::vector<VulkanDescriptorSetLayoutPtr> descriptorSetLayouts,
                       std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions,
                       const VkVertexInputBindingDescription& vertexInputBindingDescription)
    : m_programName(std::move(programName))
    , m_shaderNames(std::move(shaderNames))
    , m_descriptorSetLayouts(std::move(descriptorSetLayouts))
    , m_vertexInputAttributeDescriptions(std::move(vertexInputAttributeDescriptions))
    , m_vertexInputBindingDescription(vertexInputBindingDescription)
{

}

std::optional<VulkanDescriptorSetLayout::BindingDetails>
ProgramDef::GetBindingDetailsByName(const std::string& inputName) const
{
    for (const auto& descriptorSet : m_descriptorSetLayouts)
    {
        for (const auto& bindingDetails : descriptorSet->GetBindingDetails())
        {
            if (bindingDetails.name == inputName)
            {
                return bindingDetails;
            }
        }
    }

    return std::nullopt;
}

std::vector<VkDescriptorSetLayout> ProgramDef::GetVkDescriptorSetLayouts() const
{
    std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;

    std::transform(m_descriptorSetLayouts.begin(), m_descriptorSetLayouts.end(), std::back_inserter(vkDescriptorSetLayouts),
       [](const auto& layout) {
            return layout->GetVkDescriptorSetLayout();
        }
    );

    return vkDescriptorSetLayouts;
}

std::optional<std::vector<VulkanDescriptorSetPtr>>
ProgramDef::CreateDescriptorSets(const Common::ILogger::Ptr& logger, const DescriptorSetsPtr& descriptorSets) const
{
    uint32_t setIndex = 0;

    std::vector<VulkanDescriptorSetPtr> createdDescriptorSets;

    for (const auto& descriptorSetLayout : m_descriptorSetLayouts)
    {
        const auto descriptorSetOpt = descriptorSets->AllocateDescriptorSet(descriptorSetLayout, std::format("{}-{}", m_programName, setIndex));
        if (!descriptorSetOpt.has_value())
        {
            logger->Log(Common::LogLevel::Error, "ProgramDef: Failed to allocate a descriptor set");
            // Note that no cleanup is needed as descriptor sets are cleaned up by their pool, not individually
            return std::nullopt;
        }

        createdDescriptorSets.push_back(*descriptorSetOpt);
        setIndex++;
    }

    return createdDescriptorSets;
}

}
