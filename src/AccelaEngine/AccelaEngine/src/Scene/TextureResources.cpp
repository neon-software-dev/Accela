/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "TextureResources.h"
#include "PackageResources.h"

#include <Accela/Render/IRenderer.h>

#include <Accela/Platform/Text/IText.h>
#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Thread/ThreadUtil.h>
#include <Accela/Common/Thread/ResultMessage.h>

#include <cstring>

namespace Accela::Engine
{

struct TextureResultMessage : public Common::ResultMessage<Render::TextureId>
{
    TextureResultMessage()
        : Common::ResultMessage<Render::TextureId>("TextureResultMessage")
    { }
};

struct TextRenderResultMessage : public Common::ResultMessage<std::expected<TextRender, bool>>
{
    TextRenderResultMessage()
        : Common::ResultMessage<std::expected<TextRender, bool>>("TextRenderResultMessage")
    { }
};

TextureResources::TextureResources(Common::ILogger::Ptr logger,
                                   PackageResourcesPtr packages,
                                   std::shared_ptr<Render::IRenderer> renderer,
                                   std::shared_ptr<Platform::IFiles> files,
                                   std::shared_ptr<Platform::IText> text,
                                   std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_packages(std::move(packages))
    , m_renderer(std::move(renderer))
    , m_files(std::move(files))
    , m_text(std::move(text))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::TextureId> TextureResources::LoadPackageTexture(const PackageResourceIdentifier& resource,
                                                                    const TextureLoadConfig& loadConfig,
                                                                    ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadPackageTexture(resource, loadConfig, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadPackageTexture(const PackageResourceIdentifier& resource,
                                                         const TextureLoadConfig& loadConfig,
                                                         ResultWhen resultWhen)
{
    return LoadPackageTexture({resource}, loadConfig, resource.GetUniqueName(), resultWhen);
}

std::future<Render::TextureId> TextureResources::LoadPackageCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources,
                                                                        const TextureLoadConfig& loadConfig,
                                                                        const std::string& tag,
                                                                        ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadPackageCubeTexture(resources, loadConfig, tag, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadPackageCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources,
                                                             const TextureLoadConfig& loadConfig,
                                                             const std::string& tag,
                                                             ResultWhen resultWhen)
{
    std::vector<PackageResourceIdentifier> resourcesVec;
    std::ranges::copy(resources, std::back_inserter(resourcesVec));

    return LoadPackageTexture(resourcesVec, loadConfig, tag, resultWhen);
}

std::future<Render::TextureId> TextureResources::LoadCustomTexture(const Common::ImageData::Ptr& imageData,
                                                                   const TextureLoadConfig& loadConfig,
                                                                   const std::string& tag,
                                                                   ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadCustomTexture(imageData, loadConfig, tag, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadCustomTexture(const Common::ImageData::Ptr& imageData,
                                                        const TextureLoadConfig& loadConfig,
                                                        const std::string& tag,
                                                        ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading custom texture resource: {}", tag);

    return LoadTexture(TextureData(imageData), loadConfig, tag, resultWhen);
}

std::future<std::expected<TextRender, bool>> TextureResources::RenderText(const std::string& text, const Platform::TextProperties& properties, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextRenderResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextRenderResultMessage>(_message)->SetResult(
            OnRenderText(text, properties, resultWhen)
        );
    });

    return messageFuture;
}

std::expected<TextRender, bool> TextureResources::OnRenderText(const std::string& text,
                                                               const Platform::TextProperties& properties,
                                                               ResultWhen resultWhen)
{
    const std::string tag = "TextRender";

    if (!m_text->IsFontLoaded(properties.fontFileName, properties.fontSize))
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::OnRenderText: Font is not loaded: {}x{}", properties.fontFileName, properties.fontSize);
        return std::unexpected(false);
    }

    //
    // Have the platform render the text to an image
    //
    const auto renderedTextExpect = m_text->RenderText(text, properties);
    if (!renderedTextExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "TextureResources::OnRenderText: Failed to render text");
        return std::unexpected(false);
    }

    //
    // Create and record the texture
    //
    const auto textureId = m_renderer->GetIds()->textureIds.GetId();

    const auto texture = Render::Texture::FromImageData(
        textureId,
        1,
        false,
        renderedTextExpect->imageData,
        tag
    );
    if (!texture)
    {
        m_logger->Log(Common::LogLevel::Error, "TextureResources::OnRenderText: Failed to create texture object");
        return std::unexpected(false);
    }

    const auto textureView = Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT());

    auto textureSampler = Render::TextureSampler(Render::TextureSampler::DEFAULT(), Render::CLAMP_ADDRESS_MODE);

    // Use nearest sampling for text renders. Scenarios such as the perf monitor where the width of the text slightly
    // changes as the text at the end of a texture changes causes rasterization/sampling changes that cause text renders
    // to fluctuate when viewed close up, otherwise
    textureSampler.minFilter = Render::SamplerFilterMode::Nearest;
    textureSampler.magFilter = Render::SamplerFilterMode::Nearest;

    {
        std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

        m_textures.insert({textureId, RegisteredTexture{*texture}});
    }

    //
    // Send the texture to the renderer
    //
    std::future<bool> transferFuture = m_renderer->CreateTexture(*texture, textureView, textureSampler);

    if (resultWhen == ResultWhen::FullyLoaded && !transferFuture.get())
    {
        m_logger->Log(Common::LogLevel::Error, "TextureResources::OnRenderText: Renderer failed to create texture");
        DestroyTexture(textureId);
        return std::unexpected(false);
    }

    TextRender textRender{};
    textRender.textureId = textureId;
    textRender.textPixelWidth = renderedTextExpect->textPixelWidth;
    textRender.textPixelHeight = renderedTextExpect->textPixelHeight;

    return textRender;
}

Render::TextureId TextureResources::LoadPackageTexture(const std::vector<PackageResourceIdentifier>& resources,
                                                       const TextureLoadConfig& loadConfig,
                                                       const std::string& tag,
                                                       ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading package texture resource: {}", tag);

    //
    // Fetch the package for each resource
    //
    std::vector<Platform::PackageSource::Ptr> packages;

    for (const auto& resource : resources)
    {
        const auto package = m_packages->GetPackageSource(*resource.GetPackageName());
        if (!package)
        {
            m_logger->Log(Common::LogLevel::Error,
              "TextureResources::LoadPackageTexture: No such package: {}", resource.GetPackageName()->name);
            return Render::TextureId::Invalid();
        }

        packages.push_back(*package);
    }

    //
    // Load the texture data from the packages
    //
    TextureData textureData{};

    for (unsigned int x = 0; x < resources.size(); ++x)
    {
        const auto resourcePackage = packages.at(x);
        const auto resourceName = resources.at(x).GetResourceName();

        const auto textureBytesExpect = resourcePackage->GetTextureData(resourceName);
        const auto textureDataFormatHint = resourcePackage->GetTextureFormatHint(resourceName);
        if (!textureBytesExpect || !textureDataFormatHint)
        {
            m_logger->Log(Common::LogLevel::Error,
              "TextureResources::LoadPackageTexture: Failed to read texture: {}", resources.at(x).GetUniqueName());
            return Render::TextureId::Invalid();
        }

        const auto textureDataExpect = m_files->LoadTexture(*textureBytesExpect, *textureDataFormatHint);
        if (!textureDataExpect)
        {
            m_logger->Log(Common::LogLevel::Error,
                "TextureResources::LoadPackageTexture: Failed to convert texture to an image: {}", resources.at(x).GetUniqueName());
            return Render::TextureId::Invalid();
        }

        textureData.textureImages.push_back(*textureDataExpect);
    }

    //
    // Create and record the texture
    //
    return LoadTexture(textureData, loadConfig, tag, resultWhen);
}

Render::TextureId TextureResources::LoadTexture(const TextureData& textureData,
                                                const TextureLoadConfig& loadConfig,
                                                const std::string& tag,
                                                ResultWhen resultWhen)
{
    //
    // Create and record the texture
    //
    const auto textureId = m_renderer->GetIds()->textureIds.GetId();
    const auto textureExpect = ToRenderTexture(textureId, textureData, tag);

    if (!textureExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::LoadTexture: Failed to create render texture: {}", tag);
        m_renderer->GetIds()->textureIds.ReturnId(textureId);
        return Render::TextureId::Invalid();
    }

    auto texture = *textureExpect;

    if (texture.numLayers == 1)
    {
        texture.SetFullMipLevels();
    }

    if (loadConfig.numMipLevels)
    {
        texture.numMipLevels = *loadConfig.numMipLevels;
    }

    Render::TextureView textureView;
    if (texture.numLayers == 1)
    {
        textureView = Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT());
    }
    else
    {
        textureView = Render::TextureView::ViewAsCube(Render::TextureView::DEFAULT());
    }

    {
        std::lock_guard<std::mutex> texturesLock(m_texturesMutex);
        m_textures.insert({textureId, RegisteredTexture{texture}});
    }

    Render::UVAddressMode uvAddressMode = Render::CLAMP_ADDRESS_MODE;
    if (loadConfig.uvAddressMode)
    {
        uvAddressMode = *loadConfig.uvAddressMode;
    }

    const auto textureSampler = Render::TextureSampler(Render::TextureSampler::DEFAULT(), uvAddressMode);

    //
    // Send the texture to the renderer
    //
    std::future<bool> transferFuture = m_renderer->CreateTexture(texture, textureView, textureSampler);

    if (resultWhen == ResultWhen::FullyLoaded && !transferFuture.get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::LoadTexture: Renderer failed to create texture: {}", tag);
        DestroyTexture(textureId);
        return Render::TextureId::Invalid();
    }

    return textureId;
}

std::expected<Render::Texture, bool> TextureResources::ToRenderTexture(Render::TextureId textureId, const TextureData& textureData, const std::string& tag)
{
    const auto imageData = TextureDataToImageData(textureData);

    const bool cubicTexture = imageData->GetNumLayers() == 6;

    const auto textureExpect = Render::Texture::FromImageData(
        textureId,
        imageData->GetNumLayers(),
        cubicTexture,
        imageData,
        tag
    );

    if (!textureExpect)
    {
        return std::unexpected(false);
    }

    return *textureExpect;
}

Common::ImageData::Ptr TextureResources::TextureDataToImageData(const TextureData& textureData)
{
    //
    // If the texture is one image, then just return that one image
    //
    if (textureData.textureImages.size() == 1)
    {
        return textureData.textureImages[0];
    }

    //
    // Otherwise, combine the texture's images into a new, tightly packed, image
    //
    std::vector<std::byte> combinedImageData(textureData.textureImages[0]->GetTotalByteSize() * textureData.textureImages.size());

    for (unsigned int x = 0; x < textureData.textureImages.size(); ++x)
    {
        memcpy(
            (void*)(combinedImageData.data() + (textureData.textureImages[x]->GetTotalByteSize() * x)),
            (void*)textureData.textureImages[x]->GetPixelBytes().data(),
            textureData.textureImages[x]->GetTotalByteSize()
        );
    }

    return std::make_shared<Common::ImageData>(
        combinedImageData,
        6,
        textureData.textureImages[0]->GetPixelWidth(),
        textureData.textureImages[0]->GetPixelHeight(),
        textureData.textureImages[0]->GetPixelFormat());
}

std::optional<Render::Texture> TextureResources::GetLoadedTextureData(const Render::TextureId& textureId) const
{
    std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

    const auto it2 = m_textures.find(textureId);
    if (it2 != m_textures.cend())
    {
        return it2->second.texture;
    }

    return std::nullopt;
}

void TextureResources::DestroyTexture(const Render::TextureId& textureId)
{
    if (!textureId.IsValid()) { return; }

    std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

    const auto it = m_textures.find(textureId);
    if (it == m_textures.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Debug, "TextureResources: Destroying texture resource: {}", it->second.texture.tag);

    //
    // Destroy any texture data
    //
    const auto texturesIt = m_textures.find(textureId);
    if (texturesIt == m_textures.cend())
    {
        return;
    }

    m_renderer->DestroyTexture(textureId);

    m_textures.erase(texturesIt);
}

void TextureResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Destroying all texture resources");

    while (!m_textures.empty())
    {
        DestroyTexture(m_textures.cbegin()->first);
    }
}

}
