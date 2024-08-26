/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERINIT_H
#define ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERINIT_H

#include "Shader/ShaderSpec.h"

#include <vector>

namespace Accela::Render
{
    enum class OutputMode
    {
        Display,
        HeadsetRequired,
        HeadsetOptional
    };

    [[nodiscard]] inline bool IsHeadsetOutputMode(OutputMode outputMode)
    {
        return outputMode == OutputMode::HeadsetRequired || outputMode == OutputMode::HeadsetOptional;
    }

    struct RenderInit
    {
        OutputMode outputMode{OutputMode::Display};
        std::vector<ShaderSpec> shaders;
    };
}

#endif //ACCELAENGINE_ACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERINIT_H
