/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_SHADER_SHADERS
#define LIBACCELARENDERERVK_SRC_SHADER_SHADERS

#include "IShaders.h"

#include <Accela/Common/Log/ILogger.h>

#include <string>
#include <unordered_map>

namespace Accela::Render
{
    class Shaders : public IShaders
    {
        public:

            Shaders(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs);

            //
            // IShaders
            //
            bool LoadShader(const ShaderSpec& shaderSpec) override;
            [[nodiscard]] std::optional<VulkanShaderModulePtr> GetShaderModule(const std::string& shaderFileName) const override;
            void Destroy() override;

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;

            // Shader file name -> Shader module
            std::unordered_map<std::string, VulkanShaderModulePtr> m_loadedShaders;
    };
}

#endif //LIBACCELARENDERERVK_SRC_SHADER_SHADERS
