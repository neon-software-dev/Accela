/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldResources.h"
#include "EngineAssets.h"
#include "TextureResources.h"
#include "MeshResources.h"
#include "MaterialResources.h"
#include "AudioResources.h"
#include "FontResources.h"
#include "ModelResources.h"

#include "../Audio/AudioManager.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Render/Mesh/BoneMesh.h>
#include <Accela/Render/Mesh/StaticMesh.h>

#include <Accela/Platform/File/IFiles.h>
#include <Accela/Platform/Text/IText.h>

#include <Accela/Render/IRenderer.h>

#include <algorithm>

namespace Accela::Engine
{

WorldResources::WorldResources(Common::ILogger::Ptr logger,
                               std::shared_ptr<Render::IRenderer> renderer,
                               std::shared_ptr<Platform::IFiles> files,
                               std::shared_ptr<IEngineAssets> assets,
                               std::shared_ptr<Platform::IText> text,
                               AudioManagerPtr audioManager)
    : m_logger(std::move(logger))
    , m_threadPool(std::make_shared<Common::MessageDrivenThreadPool>("Resources", 4))
    , m_renderer(std::move(renderer))
    , m_files(std::move(files))
    , m_assets(std::move(assets))
    , m_text(std::move(text))
    , m_audioManager(std::move(audioManager))
    , m_textures(std::make_shared<TextureResources>(m_logger, m_renderer, m_assets, m_files, m_text, m_threadPool))
    , m_meshes(std::make_shared<MeshResources>(m_logger, m_textures, m_renderer, m_assets, m_files, m_threadPool))
    , m_materials(std::make_shared<MaterialResources>(m_logger, m_renderer, m_threadPool))
    , m_audio(std::make_shared<AudioResources>(m_logger, m_audioManager))
    , m_fonts(std::make_shared<FontResources>(m_logger, m_text, m_threadPool))
    , m_models(std::make_shared<ModelResources>(m_logger, m_renderer, m_files, m_threadPool))
{

}

ITextureResources::Ptr WorldResources::Textures() const { return m_textures; }
IMeshResources::Ptr WorldResources::Meshes() const { return m_meshes; }
IMaterialResources::Ptr WorldResources::Materials() const { return m_materials; }
IAudioResources::Ptr WorldResources::Audio() const { return m_audio; }
IFontResources::Ptr WorldResources::Fonts() const { return m_fonts; }
IModelResources::Ptr WorldResources::Models() const { return m_models; }

void WorldResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "WorldResources: Destroying all resources");

    m_textures->DestroyAll();
    m_meshes->DestroyAll();
    m_materials->DestroyAll();
    m_audio->DestroyAll();
    m_fonts->DestroyAll();
    m_models->DestroyAll();
}

}
