/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "RenderTarget.h"

namespace Accela::Engine
{

Render::TextureView TextureViewForLayerCount(Render::TextureView::Aspect aspect, uint32_t layerCount)
{
    //
    // If we're creating single layer render textures for desktop mode, our view of those textures is as a simple
    // one layer 2D image. If we created multiple layer textures for rendering in VR mode, we view the texture as
    // a texture array over all the texture's layers.
    //
    if (layerCount == 1)
    {
        return Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT, aspect);
    }
    else
    {
        return Render::TextureView::ViewAs2DArray(Render::TextureView::DEFAULT, aspect, Render::TextureView::Layer(0, layerCount));
    }
}

bool RenderTarget::Create(const Render::IRenderer::Ptr& renderer, const Render::RenderSettings& renderSettings)
{
    using namespace Accela::Render;

    //
    // Destroy any previous framebuffer+textures
    //
    Destroy(renderer);

    //
    // Setup
    //
    uint32_t renderLayerCount = 1;

    // If we're presenting to a headset, create two layers for each render target texture, to hold the output for each eye
    if (renderSettings.presentToHeadset)
    {
        renderLayerCount = 2;
    }

    const auto attachmentTextureSampler = TextureSampler(CLAMP_ADDRESS_MODE);

    //
    // Color Attachment
    //
    colorAttachment = renderer->GetIds()->textureIds.GetId();

    const auto colorAttachmentTexture = Texture::Empty(
        colorAttachment,
        TextureUsage::ColorAttachment,
        renderSettings.resolution,
        renderLayerCount,
        "Color"
    );

    // NOTE: This attachment is different from the others below: It's always viewed as a 2D array, in order
    // to have a swap chain blit pass with a consistent sampler for reading from this attachment (always a
    // sampler2DArray, no matter whether we're in Desktop or VR mode).
    const auto colorAttachmentTextureView = TextureView::ViewAs2DArray(
        TextureView::DEFAULT,
        TextureView::Aspect::ASPECT_COLOR_BIT,
        TextureView::Layer(0, renderLayerCount)
    );

    if (!renderer->CreateTexture(colorAttachmentTexture,colorAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(colorAttachment);
        return false;
    }

    //
    // Position Attachment
    //
    positionAttachment = renderer->GetIds()->textureIds.GetId();

    const auto positionAttachmentTexture = Texture::Empty(
        positionAttachment,
        TextureUsage::InputAttachment_RGBA16_SFLOAT,
        renderSettings.resolution,
        renderLayerCount,
        "Position"
    );

    const TextureView positionAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(positionAttachmentTexture, positionAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(positionAttachment);
        return false;
    }

    //
    // Normal Attachment
    //
    normalAttachment = renderer->GetIds()->textureIds.GetId();

    const auto normalAttachmentTexture = Texture::Empty(
        normalAttachment,
        TextureUsage::InputAttachment_RGBA16_SFLOAT,
        renderSettings.resolution,
        renderLayerCount,
        "Normal"
    );

    const TextureView normalAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(normalAttachmentTexture, normalAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(normalAttachment);
        return false;
    }

    //
    // Material Attachment
    //
    materialAttachment = renderer->GetIds()->textureIds.GetId();

    const auto materialAttachmentTexture = Texture::Empty(
        materialAttachment,
        TextureUsage::InputAttachment_R32_UINT,
        renderSettings.resolution,
        renderLayerCount,
        "Material"
    );

    const TextureView materialAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(materialAttachmentTexture, materialAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(normalAttachment);
        return false;
    }

    //
    // Ambient Attachment
    //
    ambientAttachment = renderer->GetIds()->textureIds.GetId();

    const auto ambientAttachmentTexture = Texture::Empty(
        ambientAttachment,
        TextureUsage::ColorAttachment,
        renderSettings.resolution,
        renderLayerCount,
        "Ambient"
    );

    const TextureView ambientAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(ambientAttachmentTexture, ambientAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(ambientAttachment);
        return false;
    }

    //
    // Diffuse Attachment
    //
    diffuseAttachment = renderer->GetIds()->textureIds.GetId();

    const auto diffuseAttachmentTexture = Texture::Empty(
        diffuseAttachment,
        TextureUsage::ColorAttachment,
        renderSettings.resolution,
        renderLayerCount,
        "Diffuse"
    );

    const TextureView diffuseAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(diffuseAttachmentTexture, diffuseAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(diffuseAttachment);
        return false;
    }

    //
    // Specular Attachment
    //
    specularAttachment = renderer->GetIds()->textureIds.GetId();

    const auto specularAttachmentTexture = Texture::Empty(
        specularAttachment,
        TextureUsage::ColorAttachment,
        renderSettings.resolution,
        renderLayerCount,
        "Specular"
    );

    const auto specularAttachmentTextureView  = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_COLOR_BIT, renderLayerCount);

    if (!renderer->CreateTexture(specularAttachmentTexture, specularAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(specularAttachment);
        return false;
    }

    //
    // Depth Attachment
    //
    depthAttachment = renderer->GetIds()->textureIds.GetId();

    const auto depthAttachmentTexture = Texture::Empty(
        depthAttachment,
        TextureUsage::DepthAttachment,
        renderSettings.resolution,
        renderLayerCount,
        "Depth"
    );

    const auto depthAttachmentTextureView = TextureViewForLayerCount(Render::TextureView::Aspect::ASPECT_DEPTH_BIT, renderLayerCount);

    if (!renderer->CreateTexture(depthAttachmentTexture, depthAttachmentTextureView, attachmentTextureSampler, false).get())
    {
        renderer->GetIds()->textureIds.ReturnId(depthAttachment);
        return false;
    }

    frameBuffer = renderer->GetIds()->frameBufferIds.GetId();
    if (!renderer->CreateFrameBuffer(
        frameBuffer,
        {
            colorAttachment,
            positionAttachment,
            normalAttachment,
            materialAttachment,
            ambientAttachment,
            diffuseAttachment,
            specularAttachment,
            depthAttachment
        }).get())
    {
        renderer->GetIds()->frameBufferIds.ReturnId(frameBuffer);
        return false;
    }

    return true;
}

void RenderTarget::Destroy(const Render::IRenderer::Ptr& renderer)
{
    if (frameBuffer.IsValid()) { renderer->DestroyFrameBuffer(frameBuffer); frameBuffer = {}; }
    if (depthAttachment.IsValid()) { renderer->DestroyTexture(depthAttachment); depthAttachment = {}; }
    if (specularAttachment.IsValid()) { renderer->DestroyTexture(specularAttachment); specularAttachment = {}; }
    if (diffuseAttachment.IsValid()) { renderer->DestroyTexture(diffuseAttachment); diffuseAttachment = {}; }
    if (colorAttachment.IsValid()) { renderer->DestroyTexture(colorAttachment); colorAttachment= {}; }
    if (normalAttachment.IsValid()) { renderer->DestroyTexture(normalAttachment); normalAttachment = {}; }
    if (materialAttachment.IsValid()) { renderer->DestroyTexture(materialAttachment); materialAttachment = {}; }
    if (positionAttachment.IsValid()) { renderer->DestroyTexture(positionAttachment); positionAttachment = {}; }
    if (ambientAttachment.IsValid()) { renderer->DestroyTexture(ambientAttachment); ambientAttachment = {}; }
}

}