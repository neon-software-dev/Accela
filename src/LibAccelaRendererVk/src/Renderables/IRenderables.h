/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERABLES_IRENDERABLES_H
#define LIBACCELARENDERERVK_SRC_RENDERABLES_IRENDERABLES_H

#include "SpriteRenderables.h"
#include "ObjectRenderables.h"
#include "TerrainRenderables.h"

#include <Accela/Render/Task/WorldUpdate.h>

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    class IRenderables
    {
        public:

            virtual ~IRenderables() = default;

            virtual bool Initialize() = 0;
            virtual void Destroy() = 0;

            virtual void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) = 0;

            [[nodiscard]] virtual const SpriteRenderables& GetSprites() const = 0;
            [[nodiscard]] virtual const ObjectRenderables& GetObjects() const = 0;
            [[nodiscard]] virtual const TerrainRenderables& GetTerrain() const = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERABLES_IRENDERABLES_H
