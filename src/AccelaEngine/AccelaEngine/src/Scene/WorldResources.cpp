/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldResources.h"
#include "PackageResources.h"
#include "TextureResources.h"
#include "MeshResources.h"
#include "MaterialResources.h"
#include "AudioResources.h"
#include "FontResources.h"
#include "ModelResources.h"

#include "../Audio/AudioManager.h"

#include <Accela/Platform/File/IFiles.h>
#include <Accela/Platform/Text/IText.h>

#include <Accela/Render/IRenderer.h>

#include <Accela/Common/Thread/ResultMessage.h>

#include <algorithm>

namespace Accela::Engine
{

WorldResources::WorldResources(Common::ILogger::Ptr logger,
                               std::shared_ptr<Render::IRenderer> renderer,
                               std::shared_ptr<Platform::IFiles> files,
                               std::shared_ptr<Platform::IText> text,
                               AudioManagerPtr audioManager)
    : m_logger(std::move(logger))
    , m_threadPool(std::make_shared<Common::MessageDrivenThreadPool>("Resources", 4)) // TODO Perf: Adjust pool size
    , m_renderer(std::move(renderer))
    , m_files(std::move(files))
    , m_text(std::move(text))
    , m_audioManager(std::move(audioManager))
    , m_packages(std::make_shared<PackageResources>(m_logger, m_files, m_threadPool))
    , m_textures(std::make_shared<TextureResources>(m_logger, m_packages, m_renderer, m_files, m_text, m_threadPool))
    , m_meshes(std::make_shared<MeshResources>(m_logger, m_textures, m_renderer, m_files, m_threadPool))
    , m_materials(std::make_shared<MaterialResources>(m_logger, m_textures, m_renderer, m_threadPool))
    , m_audio(std::make_shared<AudioResources>(m_logger, m_packages, m_audioManager, m_threadPool))
    , m_fonts(std::make_shared<FontResources>(m_logger, m_packages, m_text, m_threadPool))
    , m_models(std::make_shared<ModelResources>(m_logger, m_packages, m_renderer, m_files, m_threadPool))
{

}

IPackageResources::Ptr WorldResources::Packages() const { return m_packages; }
ITextureResources::Ptr WorldResources::Textures() const { return m_textures; }
IMeshResources::Ptr WorldResources::Meshes() const { return m_meshes; }
IMaterialResources::Ptr WorldResources::Materials() const { return m_materials; }
IAudioResources::Ptr WorldResources::Audio() const { return m_audio; }
IFontResources::Ptr WorldResources::Fonts() const { return m_fonts; }
IModelResources::Ptr WorldResources::Models() const { return m_models; }

std::future<bool> WorldResources::EnsurePackageResources(const PackageName& packageName, ResultWhen resultWhen)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnEnsurePackageResources(packageName, resultWhen)
        );
    });

    return messageFuture;
}

bool WorldResources::OnEnsurePackageResources(const PackageName& packageName, ResultWhen resultWhen) const
{
    m_logger->Log(Common::LogLevel::Info, "---WorldResources: Opening and loading package: {}---", packageName.name);

    //
    // Open the package's data and register the package source
    //
    const auto package = m_packages->GetPackageSource(packageName);
    if (!package)
    {
        if (!m_packages->OpenAndRegisterPackage(packageName).get())
        {
            m_logger->Log(Common::LogLevel::Error,
              "WorldResources::OnOpenAndLoadPackage: Failed to open/register package: {}", packageName.name);
            return false;
        }
    }

    //
    // Load all the package's resource.
    //
    // Order here matters; for example materials and models depend on textures.
    //
    // Note that we're also not bailing out if any particular step fails, we're just
    // trying to load as much of the package as we successfully can.
    //

    if (!m_audio->LoadAllAudio(packageName).get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldResources::OnOpenAndLoadPackage: Failed to load all audio for package: {}", packageName.name);
    }

    // Note that we're by default only loading sizes 8 through 20 of each font.
    // TODO: Revisit?
    if (!m_fonts->LoadAllFonts(packageName, 8, 20).get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldResources::OnOpenAndLoadPackage: Failed to load all fonts for package: {}", packageName.name);
    }

    // TODO: When packages contain meshes
    /*if (!m_meshes->LoadAllMeshes(packageName, resultWhen).get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldResources::OnOpenAndLoadPackage: Failed to load all meshes for package: {}", packageName.name);
    }*/

    // TODO: When packages contain materials
    /*if (!m_materials->LoadAllMaterials(packageName, resultWhen).get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldResources::OnOpenAndLoadPackage: Failed to load all materials for package: {}", packageName.name);
    }*/

    if (!m_models->LoadAllModels(packageName, resultWhen).get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "WorldResources::OnOpenAndLoadPackage: Failed to load all models for package: {}", packageName.name);
    }

    m_logger->Log(Common::LogLevel::Info, "---WorldResources: Finished with package: {}---", packageName.name);

    return true;
}

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
