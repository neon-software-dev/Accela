/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_SRC_TASK_RENDERTASKS_H
#define LIBACCELARENDERER_SRC_TASK_RENDERTASKS_H

#include <Accela/Render/Id.h>
#include <Accela/Render/RenderInit.h>
#include <Accela/Render/RenderSettings.h>
#include <Accela/Render/Task/RenderTask.h>
#include <Accela/Render/Task/WorldUpdate.h>
#include <Accela/Render/Graph/RenderGraph.h>
#include <Accela/Render/Texture/Texture.h>
#include <Accela/Render/Texture/TextureView.h>
#include <Accela/Render/Texture/TextureSampler.h>
#include <Accela/Render/Mesh/Mesh.h>

#include <Accela/Common/ImageData.h>

namespace Accela::Render
{
    using RenderTask_Initialize = DataRenderTask<RenderTaskType::Initialize, RenderInit, RenderSettings>;
    using RenderTask_Shutdown = DataRenderTask<RenderTaskType::Shutdown>;
    using RenderTask_RenderFrame = DataRenderTask<RenderTaskType::RenderFrame, RenderGraph::Ptr>;
    using RenderTask_CreateTexture = DataRenderTask<RenderTaskType::CreateTexture, Texture, TextureView, TextureSampler>;
    using RenderTask_UpdateTexture = DataRenderTask<RenderTaskType::UpdateTexture, TextureId, Common::ImageData::Ptr>;
    using RenderTask_DestroyTexture = DataRenderTask<RenderTaskType::DestroyTexture, TextureId>;
    using RenderTask_CreateMesh = DataRenderTask<RenderTaskType::CreateMesh, Mesh::Ptr, MeshUsage>;
    using RenderTask_DestroyMesh = DataRenderTask<RenderTaskType::DestroyMesh, MeshId>;
    using RenderTask_CreateMaterial = DataRenderTask<RenderTaskType::CreateMaterial, Material::Ptr>;
    using RenderTask_DestroyMaterial = DataRenderTask<RenderTaskType::DestroyMaterial, MaterialId>;
    using RenderTask_CreateRenderTarget = DataRenderTask<RenderTaskType::CreateRenderTarget, RenderTargetId, std::string>;
    using RenderTask_DestroyRenderTarget = DataRenderTask<RenderTaskType::DestroyRenderTarget, RenderTargetId>;
    using RenderTask_WorldUpdate = DataRenderTask<RenderTaskType::WorldUpdate, WorldUpdate>;
    using RenderTask_SurfaceChanged = DataRenderTask<RenderTaskType::SurfaceChanged>;
    using RenderTask_ChangeRenderSettings = DataRenderTask<RenderTaskType::ChangeRenderSettings, RenderSettings>;
}

#endif //LIBACCELARENDERER_SRC_TASK_RENDERTASKS_H
