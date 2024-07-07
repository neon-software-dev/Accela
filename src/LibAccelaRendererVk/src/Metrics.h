/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_METRICS_H
#define LIBACCELARENDERERVK_SRC_METRICS_H

namespace Accela::Render
{
    // Renderer general
        static constexpr char Renderer_FrameRenderTotal_Time[] = "Renderer_FrameRenderTotal_Time";
        static constexpr char Renderer_FrameRenderWork_Time[] = "Renderer_FrameRenderWork_Time";
        static constexpr char Renderer_Scene_Lights_Count[] = "Renderer_Scene_Lights_Count";
        static constexpr char Renderer_Scene_Shadow_Map_Count[] = "Renderer_Scene_Shadow_Map_Count";
        static constexpr char Renderer_Scene_Update_Time[] = "Renderer_Scene_Update_Time";

    // Object renderer
        static constexpr char Renderer_Object_Opaque_Objects_Rendered_Count[] = "Renderer_Object_Opaque_Objects_Rendered_Count";
        static constexpr char Renderer_Object_Opaque_RenderBatch_Count[] = "Renderer_Object_Opaque_RenderBatch_Count";
        static constexpr char Renderer_Object_Opaque_DrawCalls_Count[] = "Renderer_Object_Opaque_DrawCalls_Count";
        static constexpr char Renderer_Object_Transparent_Objects_Rendered_Count[] = "Renderer_Object_Transparent_Objects_Rendered_Count";
        static constexpr char Renderer_Object_Transparent_RenderBatch_Count[] = "Renderer_Object_Transparent_RenderBatch_Count";
        static constexpr char Renderer_Object_Transparent_DrawCalls_Count[] = "Renderer_Object_Transparent_DrawCalls_Count";

    // Meshes system
        static constexpr char Renderer_Meshes_Count[] = "Renderer_Meshes_Count";
        static constexpr char Renderer_Meshes_Loading_Count[] = "Renderer_Meshes_Loading_Count";
        static constexpr char Renderer_Meshes_ToDestroy_Count[] = "Renderer_Meshes_ToDestroy_Count";
        static constexpr char Renderer_Meshes_ByteSize[] = "Renderer_Meshes_ByteSize";

    // Images system
        static constexpr char Renderer_Images_Count[] = "Renderer_Images_Count";
        static constexpr char Renderer_Images_Loading_Count[] = "Renderer_Images_Loading_Count";
        static constexpr char Renderer_Images_ToDestroy_Count[] = "Renderer_Images_ToDestroy_Count";

    // Textures system
        static constexpr char Renderer_Textures_Count[] = "Renderer_Textures_Count";

    // Materials system
        static constexpr char Renderer_Materials_Count[] = "Renderer_Materials_Count";
        static constexpr char Renderer_Materials_Loading_Count[] = "Renderer_Materials_Loading_Count";
        static constexpr char Renderer_Materials_ToDestroy_Count[] = "Renderer_Materials_ToDestroy_Count";
        static constexpr char Renderer_Materials_ByteSize[] = "Renderer_Materials_ByteSize";

    // Buffers system
        static constexpr char Renderer_Buffers_Count[] = "Renderer_Buffers_Count";
        static constexpr char Renderer_Buffers_ByteSize[] = "Renderer_Buffers_ByteSize";

    // Memory usage
        static constexpr char Renderer_Memory_Usage[] = "Renderer_Memory_Usage";
        static constexpr char Renderer_Memory_Available[] = "Renderer_Memory_Available";
}

#endif //LIBACCELARENDERERVK_SRC_METRICS_H
