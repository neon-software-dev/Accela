/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERVK_SRC_PROGRAM_PROGRAMS
#define LIBACCELARENDERVK_SRC_PROGRAM_PROGRAMS

#include "IPrograms.h"

#include "../Shader/IShaders.h"

#include <Accela/Common/Log/ILogger.h>

#include <spirv_reflect.h>

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Accela::Render
{
    class Programs : public IPrograms
    {
        public:

            Programs(Common::ILogger::Ptr logger,
                     VulkanObjsPtr vulkan,
                     IShadersPtr shaders);

            void Destroy() override;

            bool CreateProgram(const std::string& programName, const std::vector<std::string>& shaders) override;
            [[nodiscard]] ProgramDefPtr GetProgramDef(const std::string& programName) const override;
            void DestroyProgram(const std::string& programName) override;

        private:

            [[nodiscard]] std::optional<std::vector<VulkanShaderModulePtr>>
                GetShaderModules(const std::vector<std::string>& shaderFileNames) const;

            //// Descriptor Sets

            [[nodiscard]] std::optional<std::vector<VulkanDescriptorSetLayoutPtr>>
                GenerateDescriptorSetLayouts(
                    const std::vector<VulkanShaderModulePtr>& shaderModules,
                    const std::string& tag) const;

            [[nodiscard]] VulkanDescriptorSetLayoutPtr
                GenerateDescriptorSetLayout(
                    const std::vector<VulkanShaderModulePtr>& shaderModules,
                    uint32_t set,
                    const std::string& tag) const;

            [[nodiscard]] static std::optional<SpvReflectDescriptorSet>
                GetModuleReflectDescriptorSet(const SpvReflectShaderModule& module, uint32_t set);

            //// Input Attribute Descriptions

            [[nodiscard]] static std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
                GenerateVertexInputDescriptions(const std::vector<VulkanShaderModulePtr>& shaderModules);

            [[nodiscard]] static std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
                GetModuleVertexInputDescriptions(const SpvReflectShaderModule& module);

            static uint32_t FormatSize(VkFormat format);

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkan;
            IShadersPtr m_shaders;

            std::unordered_map<std::string, ProgramDefPtr> m_programDefs;
    };
}

#endif //LIBACCELARENDERVK_SRC_PROGRAM_PROGRAMS
