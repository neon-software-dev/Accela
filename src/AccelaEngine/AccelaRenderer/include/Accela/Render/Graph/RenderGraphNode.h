/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODE_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <string_view>
#include <tuple>

namespace Accela::Render
{
    enum class RenderGraphNodeType
    {
        RenderScene,    // Render a scene to a texture
        Present         // Present a texture to the display
    };

    /**
     * Defines a particular work item to be performed when rendering a scene
     */
    struct ACCELA_PUBLIC RenderGraphNode
    {
        using Ptr = std::shared_ptr<RenderGraphNode>;

        virtual ~RenderGraphNode() = default;

        [[nodiscard]] virtual RenderGraphNodeType GetType() const = 0;

        template <typename T, typename ...Args>
        RenderGraphNode::Ptr AndThen(Args&&... args)
        {
            auto node = std::make_shared<T>(std::forward<Args>(args)...);
            children.push_back(node);
            return node;
        }

        template <typename NodeType, typename Func>
        auto Apply(Func func)
        {
            return std::apply(func, ((NodeType*)(this))->data);
        }

        std::vector<RenderGraphNode::Ptr> children;
    };

    /**
     * A RenderGraphNode that has data associated with it
     */
    template <RenderGraphNodeType NodeType, typename... Args>
    struct DataRenderGraphNode : public RenderGraphNode
    {
        explicit DataRenderGraphNode(Args... args)
            : data(args...)
        { }

        [[nodiscard]] RenderGraphNodeType GetType() const override { return NodeType; };

        std::tuple<Args...> data;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_GRAPH_RENDERGRAPHNODE_H
