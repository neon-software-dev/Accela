#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERSPEC
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERSPEC

#include "ShaderType.h"

#include <string>
#include <vector>

namespace Accela::Render
{
    struct ShaderSpec
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
