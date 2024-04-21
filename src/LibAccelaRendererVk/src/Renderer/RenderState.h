/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_RENDERSTATE_H
#define LIBACCELARENDERERVK_SRC_RENDERER_RENDERSTATE_H

#include "../ForwardDeclares.h"
#include "../InternalId.h"

#include <string>
#include <optional>
#include <unordered_map>

namespace Accela::Render
{
    struct RenderState
    {
        // Non-Descriptor Set Bound Data
        std::optional<ProgramDefPtr> programDef;
        std::optional<VulkanPipelinePtr> pipeline;

        std::optional<BufferPtr> vertexBuffer;
        std::optional<BufferPtr> indexBuffer;

        // DS Set 0 - Global Data
        bool set0Invalidated{true};

        // DS Set 1 - Renderer Data
        bool set1Invalidated{true};

        // DS Set 2 - Material Data
        bool set2Invalidated{true};
        std::optional<BufferId> materialDataBufferId;
        std::optional<std::unordered_map<std::string, TextureId>> materialTextures;

        // DS Set 3 - Draw Data
        bool set3Invalidated{true};

        void OnPipelineBound(const ProgramDefPtr& programDef, const VulkanPipelinePtr& pipeline);
        void OnVertexBufferBound(const BufferPtr& buffer);
        void OnIndexBufferBound(const BufferPtr& buffer);
        void OnSet0Bound();
        void OnSet1Bound();
        void OnSet2Bound();
        void OnSet3Bound();
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RENDERSTATE_H
