#ifndef LIBACCELAENGINE_SRC_RENDERTARGET_H
#define LIBACCELAENGINE_SRC_RENDERTARGET_H

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Id.h>

namespace Accela::Engine
{
    /**
     * Wrapper class which holds the textures and framebuffer needed to render a scene
     */
    struct RenderTarget
    {
        [[nodiscard]] bool Create(const Render::IRenderer::Ptr& renderer, const Render::RenderSettings& renderSettings);
        void Destroy(const Render::IRenderer::Ptr& renderer);

        Render::FrameBufferId frameBuffer;
        Render::TextureId colorAttachment;
        Render::TextureId positionAttachment;
        Render::TextureId normalAttachment;
        Render::TextureId materialAttachment;
        Render::TextureId ambientAttachment;
        Render::TextureId diffuseAttachment;
        Render::TextureId specularAttachment;
        Render::TextureId depthAttachment;
    };
}

#endif //LIBACCELAENGINE_SRC_RENDERTARGET_H
