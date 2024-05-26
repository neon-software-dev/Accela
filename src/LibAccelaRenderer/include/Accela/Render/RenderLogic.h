#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERLOGIC_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERLOGIC_H

#include "RenderSettings.h"

#include "Util/Rect.h"

namespace Accela::Render
{
    /**
     * Calculates the render/draw portion of the window, given the window resolution, render resolution,
     * and present scaling mode.
     *
     * Note: All coordinates use (0, 0) as top left, with X and Y increasing rightwards and downwards
     *
     * Note: The result can have negative values, as the render area can be larger than the target/window area.
     *
     * @param scaling The scaling mode to apply
     * @param renderSize The size of the render area
     * @param targetSize The size of the target (window) being rendered into
     *
     * @return A rect that defines the area of the target to be rendered to
     */
    ScreenRect CalculateBlitRect(const RenderSettings& renderSettings, const USize& targetSize);
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERLOGIC_H
