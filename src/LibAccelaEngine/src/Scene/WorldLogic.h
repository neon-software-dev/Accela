/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_WORLDLOGIC_H
#define LIBACCELAENGINE_SRC_SCENE_WORLDLOGIC_H

#include <Accela/Engine/Scene/IWorldResources.h>

#include <Accela/Engine/Component/SpriteRenderableComponent.h>
#include <Accela/Engine/Component/TransformComponent.h>

#include <Accela/Render/RenderSettings.h>

#include <glm/glm.hpp>

#include <optional>

namespace Accela::Engine
{
    /**
     * Takes a window point and converts it to a render space point. If the window point doesn't fall
     * within the render area, returns std::nullopt
     *
     * @param renderSettings The current render settings
     * @param windowSize The current window size
     * @param windowPoint The window point to be converted
     *
     * @return The corresponding render area point, or std::nullopt if the point isn't contained within the render area
     */
    [[nodiscard]] std::optional<glm::vec2> WindowPointToRenderPoint(const Render::RenderSettings& renderSettings,
                                                                    const Render::USize& windowSize,
                                                                    const glm::vec2& windowPoint);

    [[nodiscard]] glm::vec2 GetVirtualToRenderRatio(const Render::RenderSettings& renderSettings,
                                                    const glm::vec2& virtualResolution);

    [[nodiscard]] glm::vec2 RenderPointToVirtualPoint(const Render::RenderSettings& renderSettings,
                                                      const glm::vec2& virtualResolution,
                                                      const glm::vec2& renderPoint);

    [[nodiscard]] glm::vec2 VirtualPointToRenderPoint(const Render::RenderSettings& renderSettings,
                                                      const glm::vec2& virtualResolution,
                                                      const glm::vec2& virtualPoint);

    template <typename S>
    [[nodiscard]] Render::Size<S> RenderSizeToVirtualSize(const Render::RenderSettings& renderSettings,
                                                          const glm::vec2& virtualResolution,
                                                          const Render::Size<S>& renderSize)
    {
        const auto virtualRatio = GetVirtualToRenderRatio(renderSettings, virtualResolution);

        return {
            (S)((float)renderSize.w * virtualRatio.x),
            (S)((float)renderSize.h * virtualRatio.y)
        };
    }

    /**
     * Determines whether a given point in render space overlaps with a specified sprite
     *
     * @param resources The IWorldResources instance
     * @param sprite The sprite component to be tested
     * @param transform The sprite's transform component
     * @param renderPoint The render area point to be tested
     *
     * @return Whether the sprite overlaps with the render point
     */
    [[nodiscard]] bool SpriteContainsPoint(const IWorldResources::Ptr& resources,
                                           const Render::RenderSettings& renderSettings,
                                           const glm::vec2& virtualResolution,
                                           const SpriteRenderableComponent& sprite,
                                           const TransformComponent& transform,
                                           const glm::vec2& virtualPoint);
}

#endif //LIBACCELAENGINE_SRC_SCENE_WORLDLOGIC_H
