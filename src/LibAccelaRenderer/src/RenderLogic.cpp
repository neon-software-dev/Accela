/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Render/RenderLogic.h>

#include <cassert>

namespace Accela::Render
{


ScreenRect CalculateBlitRect_CenterCrop(const RenderSettings& renderSettings, const USize& targetSize);
ScreenRect CalculateBlitRect_CenterInside(const RenderSettings& renderSettings, const USize& targetSize);

ScreenRect CalculateBlitRect(const RenderSettings& renderSettings, const USize& targetSize)
{
    switch (renderSettings.presentScaling)
    {
        case PresentScaling::CenterCrop: return CalculateBlitRect_CenterCrop(renderSettings, targetSize);
        case PresentScaling::CenterInside: return CalculateBlitRect_CenterInside(renderSettings, targetSize);
    }

    assert(false);
    return {};
}

ScreenRect CalculateBlitRect_CenterCrop(const RenderSettings& renderSettings, const USize& targetSize)
{
    const auto renderResolution = renderSettings.resolution;
    const float xScale = (float)targetSize.w / (float)renderResolution.w;
    const float yScale = (float)targetSize.h / (float)renderResolution.h;
    const float scale = std::max(xScale, yScale);

    const float scaledWidth = scale * (float)renderResolution.w;
    const float scaledHeight = scale * (float)renderResolution.h;

    const float left = ((float)targetSize.w - scaledWidth) / 2.0f;
    const float top = ((float)targetSize.h - scaledHeight) / 2.0f;

    return {(int)left, (int)top, (int)scaledWidth, (int)scaledHeight};
}

ScreenRect CalculateBlitRect_CenterInside(const RenderSettings& renderSettings, const USize& targetSize)
{
    const auto renderResolution = renderSettings.resolution;
    const float renderAspectRatio = (float)renderResolution.w / (float)renderResolution.h;
    const float targetAspectRatio = (float)targetSize.w / (float)targetSize.h;

    ScreenRect outRect(0, 0, (int)targetSize.w, (int)targetSize.h);

    if (renderAspectRatio >= targetAspectRatio)
    {
        outRect.h = (int)((float)renderResolution.h * ((float)targetSize.w / (float)renderResolution.w));
        outRect.y = (int)(((float)targetSize.h - (float)outRect.h) / 2.0f);
    }
    else
    {
        outRect.w = (int)((float)renderResolution.w * ((float)targetSize.h / (float)renderResolution.h));
        outRect.x = (int)(((float)targetSize.w - (float)outRect.w) / 2.0f);
    }

    return outRect;
}

}
