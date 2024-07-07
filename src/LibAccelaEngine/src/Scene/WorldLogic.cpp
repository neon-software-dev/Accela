/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldLogic.h"

#include <Accela/Render/RenderLogic.h>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>

namespace Accela::Engine
{

std::optional<glm::vec2> WindowPointToRenderPoint(const Render::RenderSettings& renderSettings,
                                                   const Render::USize& windowSize,
                                                   const glm::vec2& windowPoint)
{
    const auto blitRect = CalculateBlitRect(renderSettings, windowSize);

    const int32_t wOffset = (int32_t)blitRect.w - (int32_t)windowSize.w;
    const int32_t hOffset = (int32_t)blitRect.h - (int32_t)windowSize.h;
    const float halfWOffset = (float)wOffset / 2.0f;
    const float halfHOffset = (float)hOffset / 2.0f;

    const auto blitPoint = windowPoint + glm::vec2(halfWOffset, halfHOffset);

    if (blitPoint.x < 0.0f || blitPoint.y < 0.0f) { return std::nullopt; }
    if (blitPoint.x > (float)blitRect.w || blitPoint.y > (float)blitRect.h) { return std::nullopt; }

    const auto renderToBlitRatio = glm::vec2(
        (float)renderSettings.resolution.w / (float)blitRect.w,
        (float)renderSettings.resolution.h / (float)blitRect.h
    );

    return blitPoint * renderToBlitRatio;
}

glm::vec2 GetVirtualToRenderRatio(const Render::RenderSettings& renderSettings,
                                  const glm::vec2& virtualResolution)
{
    return {
        virtualResolution.x / (float)renderSettings.resolution.w,
        virtualResolution.y / (float)renderSettings.resolution.h
    };
}

glm::vec2 RenderPointToVirtualPoint(const Render::RenderSettings& renderSettings,
                                    const glm::vec2& virtualResolution,
                                    const glm::vec2& renderPoint)
{
    return renderPoint * GetVirtualToRenderRatio(renderSettings, virtualResolution);
}

glm::vec2 VirtualPointToRenderPoint(const Render::RenderSettings& renderSettings,
                                    const glm::vec2& virtualResolution,
                                    const glm::vec2& virtualPoint)
{
    return virtualPoint / GetVirtualToRenderRatio(renderSettings, virtualResolution);
}

// Whether the given point is "inside" the provided line. All coordinates
// are expected to be in screen/virtual space.
bool PointInsideLine(const glm::vec2& p, const std::array<glm::vec2, 2>& l)
{
    // Flip p and l coordinates from screen-space (positive y downwards) to regular
    // cartesian coordinates (positive y upwards)
    const auto flippedP = glm::vec2(p.x, -p.y);
    const auto flippedL = std::array<glm::vec2, 2>{
        glm::vec2{l[0].x, -l[0].y},
        glm::vec2{l[1].x, -l[1].y}
    };

    return
        (flippedP.x - flippedL[0].x) * (flippedL[1].y - flippedL[0].y) -
        (flippedP.y - flippedL[0].y) * (flippedL[1].x - flippedL[0].x)
        >= 0.0f;
}

// Whether the given point, is within the bounds of the rect provided. All coordinates
// are expected to be in screen/virtual space.
bool PointWithinRect(const glm::vec2& p, const std::array<glm::vec2, 4>& r)
{
    return
        PointInsideLine(p, {r[0], r[1]}) &&
        PointInsideLine(p, {r[1], r[2]}) &&
        PointInsideLine(p, {r[2], r[3]}) &&
        PointInsideLine(p, {r[3], r[0]});
}

bool SpriteContainsPoint(const IWorldResources::Ptr& resources,
                         const Render::RenderSettings& renderSettings,
                         const glm::vec2& virtualResolution,
                         const SpriteRenderableComponent& sprite,
                         const TransformComponent& transform,
                         const glm::vec2& virtualPoint)
{
    const auto textureDataOpt = resources->Textures()->GetLoadedTextureData(sprite.textureId);

    if (!textureDataOpt || !textureDataOpt->data) { return false; }

    auto pixelSize = glm::vec2(textureDataOpt->pixelSize.w, textureDataOpt->pixelSize.h);
    const auto virtualPosition = transform.GetPosition();
    const glm::vec2 scale = glm::vec2(transform.GetScale());

    auto dstVirtualSize = sprite.dstVirtualSize;
    if (!dstVirtualSize)
    {
        // If no virtual size specified, use the natural pixel size of the texture, converted to virtual size
        const auto dstRenderSize = Render::FSize(pixelSize.x, pixelSize.y);
        dstVirtualSize = RenderSizeToVirtualSize(renderSettings, virtualResolution, dstRenderSize);
    }

    const float halfSpriteVirtualWidth = (float)dstVirtualSize->w / 2.0f;
    const float halfSpriteVirtualHeight = (float)dstVirtualSize->h / 2.0f;

    // In virtual/screen space coordinate system (positive Y downwards)
    std::array<glm::vec2, 4> spriteVirtualPoints
    {
        glm::vec2{-halfSpriteVirtualWidth,-halfSpriteVirtualHeight},  // Top left
        glm::vec2{halfSpriteVirtualWidth, -halfSpriteVirtualHeight},  // Top right
        glm::vec2{halfSpriteVirtualWidth, halfSpriteVirtualHeight},   // Bottom right
        glm::vec2{-halfSpriteVirtualWidth, halfSpriteVirtualHeight}   // Bottom left
    };

    // Transform sprite points by the sprite's scale, then orientation, then position
    std::ranges::transform(spriteVirtualPoints, spriteVirtualPoints.begin(), [&](const glm::vec2& in) -> glm::vec2{
        // Scale
        auto point = in * scale;
        // Orientation
        point = glm::vec2(glm::mat4_cast(transform.GetOrientation()) * glm::vec4(point, 0, 1));
        // Translation
        point = point + glm::vec2(virtualPosition);

        return point;
    });

    return PointWithinRect(virtualPoint, spriteVirtualPoints);
}

}
