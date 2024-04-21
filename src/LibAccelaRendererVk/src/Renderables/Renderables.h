/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERALES_RENDERABLES_H
#define LIBACCELARENDERERVK_SRC_RENDERALES_RENDERABLES_H

#include "IRenderables.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Ids.h>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Render
{
    class Renderables : public IRenderables
    {
        public:

            Renderables(Common::ILogger::Ptr logger,
                        Ids::Ptr ids,
                        PostExecutionOpsPtr postExecutionOps,
                        ITexturesPtr textures,
                        IBuffersPtr buffers,
                        IMeshesPtr meshes,
                        ILightsPtr lights);

            bool Initialize() override;
            void Destroy() override;

            void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) override;

            [[nodiscard]] const SpriteRenderables& GetSprites() const override { return m_sprites; }
            [[nodiscard]] const ObjectRenderables& GetObjects() const override { return m_objects; }
            [[nodiscard]] const TerrainRenderables& GetTerrain() const override { return m_terrain; }

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            ITexturesPtr m_textures;
            IBuffersPtr m_buffers;
            IMeshesPtr m_meshes;
            ILightsPtr m_lights;

            SpriteRenderables m_sprites;
            ObjectRenderables m_objects;
            TerrainRenderables m_terrain;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERALES_RENDERABLES_H
