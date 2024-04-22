/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODES_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODES_H

#include "RenderGraphNode.h"

#include "../PresentConfig.h"
#include "../Task/RenderParams.h"
#include "../Util/Rect.h"

#include <string>

namespace Accela::Render
{
    /**
     * A node for rendering a scene to a texture
     *
     * @param std::string - The name of the scene to render
     * @param FrameBufferId - The framebuffer to be rendered into
     * @param RenderParams - The parameters controlling the render
     */
    using RenderGraphNode_RenderScene   = DataRenderGraphNode<RenderGraphNodeType::RenderScene, std::string, FrameBufferId, RenderParams>;

    /**
     * A node for presenting a texture to the display
     *
     * @param TextureId - The TextureId of the texture to be presented
     * @param PresentConfig - The parameters for controlling the presentation
     */
    using RenderGraphNode_Present       = DataRenderGraphNode<RenderGraphNodeType::Present, TextureId, PresentConfig>;
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODES_H
