/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_SHADER_ISHADERS
#define LIBACCELARENDERERVK_SRC_SHADER_ISHADERS

#include "../ForwardDeclares.h"

#include <Accela/Render/Shader/ShaderSpec.h>

#include <string>
#include <optional>

namespace Accela::Render
{
    /**
     * System which manages shaders
     */
    class IShaders
    {
        public:

            virtual ~IShaders() = default;

            /**
             * Load the specified shader into vulkan
             *
             * @param shaderSpec The definition of the shader to load
             *
             * @return Whether the load was successful
             */
            virtual bool LoadShader(const ShaderSpec& shaderSpec) = 0;

            /**
             * Retrieve the details of a previously loaded shader
             *
             * @param shaderFileName The name of the shader
             *
             * @return The shader details, or nullptr if no such shader
             */
            [[nodiscard]] virtual std::optional<VulkanShaderModulePtr> GetShaderModule(const std::string& shaderFileName) const = 0;

            /**
             * Release any resources used by this system
             */
            virtual void Destroy() = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_SHADER_ISHADERS
