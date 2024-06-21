/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H
#define LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    enum class Axis { X, Y, Z };

    // Warning: These are matched to GPU indexing of cube faces, don't reorder them
    enum class CubeFace
    {
        Right, Left, Up, Down, Back, Forward
    };

    // Attachment indices for the Offscreen Framebuffer
    static const uint32_t Offscreen_Attachment_Color = 0;
    static const uint32_t Offscreen_Attachment_Position = 1;
    static const uint32_t Offscreen_Attachment_Normal = 2;
    static const uint32_t Offscreen_Attachment_Material = 3;
    static const uint32_t Offscreen_Attachment_Ambient = 4;
    static const uint32_t Offscreen_Attachment_Diffuse = 5;
    static const uint32_t Offscreen_Attachment_Specular = 6;
    static const uint32_t Offscreen_Attachment_Depth = 7;

    // Subpass indices for the Offscreen Render Pass
    static const uint32_t GPassRenderPass_SubPass_DeferredLightingObjects = 0;
    static const uint32_t GPassRenderPass_SubPass_DeferredLightingRender = 1;
    static const uint32_t GPassRenderPass_SubPass_ForwardLightingObjects = 2;

    // Attachment indices for the Blit Framebuffer
    static const uint32_t Blit_Attachment_Color = 0;
    static const uint32_t Blit_Attachment_Depth = 1;

    // Subpass indices for the Blit Render Pass
    static const uint32_t BlitRenderPass_SubPass_Blit = 0;

    // Subpass indexes for the Shadow Render Pass
    static const uint32_t ShadowRenderPass_ShadowSubpass_Index = 0;

    // Local work group size of post effect compute shaders
    static const uint32_t POST_PROCESS_LOCAL_SIZE_X = 16;
    static const uint32_t POST_PROCESS_LOCAL_SIZE_Y = 16;
    static const uint32_t POST_PROCESS_LOCAL_SIZE_Z = 1;

    using AttachmentIndex = uint32_t;
}

#endif //LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H
