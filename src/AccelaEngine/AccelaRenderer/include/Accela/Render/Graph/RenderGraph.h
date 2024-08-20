/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPH_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPH_H

#include "RenderGraphNode.h"

#include <Accela/Common/SharedLib.h>

#include <memory>

namespace Accela::Render
{
    /**
     * Defines the work to be done to render a frame
     */
    struct ACCELA_PUBLIC RenderGraph
    {
        using Ptr = std::shared_ptr<RenderGraph>;

        RenderGraph() = default;

        explicit RenderGraph(RenderGraphNode::Ptr _root)
            : root(std::move(_root))
        { }

        template <typename T, typename ...Args>
        RenderGraphNode::Ptr StartWith(Args&&... args)
        {
            root = std::make_shared<T>(std::forward<Args>(args)...);
            return root;
        }

        RenderGraphNode::Ptr root;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPH_H
