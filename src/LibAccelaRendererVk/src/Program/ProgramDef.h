/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PROGRAM_PROGRAMDEF
#define LIBACCELARENDERERVK_SRC_PROGRAM_PROGRAMDEF

#include "../ForwardDeclares.h"

#include "../Vulkan/VulkanDescriptorSetLayout.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <optional>

namespace Accela::Render
{
    /**
     * Describes a program that the renderer can use. A program is defined by a collection
     * of shaders to be used for rendering. This class also holds a DescriptorSetLayout vector which
     * defines the inputs that the program expects. There is always one ProgramDef instance per
     * type of program that can be used for rendering.
     */
    class ProgramDef
    {
        public:

            ProgramDef(
                std::string programName,
                std::vector<std::string> shaderNames,
                std::vector<VulkanDescriptorSetLayoutPtr> descriptorSetLayouts,
                std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions,
                const VkVertexInputBindingDescription& vertexInputBindingDescription);

            [[nodiscard]] std::string GetProgramName() const { return m_programName; }
            [[nodiscard]] std::vector<std::string> GetShaderNames() const { return m_shaderNames; }
            [[nodiscard]] std::vector<VulkanDescriptorSetLayoutPtr> GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
            [[nodiscard]] std::vector<VkVertexInputAttributeDescription> GetVertexInputAttributeDescriptions() const { return m_vertexInputAttributeDescriptions; }
            [[nodiscard]] VkVertexInputBindingDescription GetVertexInputBindingDescription() const { return m_vertexInputBindingDescription; }

            [[nodiscard]] std::vector<VkDescriptorSetLayout> GetVkDescriptorSetLayouts() const;
            [[nodiscard]] std::optional<VulkanDescriptorSetLayout::BindingDetails> GetBindingDetailsByName(const std::string& inputName) const;

            [[nodiscard]] std::optional<std::vector<VulkanDescriptorSetPtr>>
                CreateDescriptorSets(const Common::ILogger::Ptr& logger, const DescriptorSetsPtr& descriptorSets) const;

        private:

            std::string m_programName;
            std::vector<std::string> m_shaderNames;
            std::vector<VulkanDescriptorSetLayoutPtr> m_descriptorSetLayouts;
            std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;
            VkVertexInputBindingDescription m_vertexInputBindingDescription;
    };
}

#endif //LIBACCELARENDERERVK_SRC_PROGRAM_PROGRAMDEF
