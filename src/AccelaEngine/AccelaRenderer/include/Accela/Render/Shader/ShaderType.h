/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERTYPE
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERTYPE

namespace Accela::Render
{
    enum class ShaderType
    {
        Vertex,
        Fragment,
        TESC,
        TESE,
        Compute
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_SHADER_SHADERTYPE
