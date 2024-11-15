/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERSPEC
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERSPEC

#include "ShaderType.h"

#include <Accela/Common/SharedLib.h>

#include <string>
#include <vector>

namespace Accela::Render
{
    struct ACCELA_PUBLIC ShaderSpec
    {
        ShaderSpec(std::string _shaderName, ShaderType _shaderType, std::vector<unsigned char> _shaderSource)
            : shaderName(std::move(_shaderName))
            , shaderType(_shaderType)
            , shaderSource(std::move(_shaderSource))
        { }

        std::string shaderName;
        ShaderType shaderType;
        std::vector<unsigned char> shaderSource;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERSPEC
