/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "TextureResources.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Render/IRenderer.h>

#include <Accela/Platform/Text/IText.h>
#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Thread/ThreadUtil.h>
#include <Accela/Common/Thread/ResultMessage.h>

#include <sstream>

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

struct BoolResultMessage : public Common::ResultMessage<bool>
{
    BoolResultMessage()
        : Common::ResultMessage<bool>("BoolResultMessage")
    { }
};

TextureResources::TextureResources(Common::ILogger::Ptr logger,
                                   std::shared_ptr<Render::IRenderer> renderer,
                                   std::shared_ptr<IEngineAssets> assets,
                                   std::shared_ptr<Platform::IFiles> files,
                                   std::shared_ptr<Platform::IText> text,
                                   std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_renderer(std::move(renderer))
    , m_assets(std::move(assets))
    , m_files(std::move(files))
    , m_text(std::move(text))
    , m_threadPool(std::move(threadPool))
{

}

std::future<bool> TextureResources::LoadAllAssetTextures(ResultWhen resultWhen)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<BoolResultMessage>(message)->SetResult(
            OnLoadAllAssetTextures(resultWhen)
        );
    });

    return messageFuture;
}

std::future<Render::TextureId> TextureResources::LoadAssetTexture(const std::string& assetTextureName, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<TextureResultMessage>(message)->SetResult(
            OnLoadAssetTexture(assetTextureName, resultWhen)
        );
    });

    return messageFuture;
}

std::future<Render::TextureId>
TextureResources::LoadAssetCubeTexture(const std::array<std::string, 6>& assetTextureNames, const std::string& tag, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<TextureResultMessage>(message)->SetResult(
            OnLoadAssetCubeTexture(assetTextureNames, tag, resultWhen)
        );
    });

    return messageFuture;
}

std::future<std::expected<TextRender, bool>>
TextureResources::RenderText(const std::string& text, const Platform::TextProperties& properties, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextRenderResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<TextRenderResultMessage>(message)->SetResult(
            OnRenderText(text, properties, resultWhen)
        );
    });

    return messageFuture;
}

bool TextureResources::OnLoadAllAssetTextures(ResultWhen resultWhen)
{
    const auto filesListExpect = m_files->ListFilesInAssetsSubdir(Platform::TEXTURES_SUBDIR);
    if (!filesListExpect)
    {
        return false;
    }

    bool allSuccess = true;

    for (const auto& file : *filesListExpect)
    {
        if (!OnLoadAssetTexture(file, resultWhen).IsValid())
        {
            allSuccess = false;
        }
    }

    return allSuccess;
}

Render::TextureId TextureResources::OnLoadAssetTexture(const std::string& assetTextureName, ResultWhen resultWhen)
{
    return OnLoadAssetTextureInternal({assetTextureName}, assetTextureName, resultWhen);
}

Render::TextureId TextureResources::OnLoadAssetCubeTexture(const std::array<std::string, 6>& assetTextureNames, const std::string& tag, ResultWhen resultWhen)
{
    std::vector<std::string> assetTextureNamesVec;
    std::ranges::copy(assetTextureNames, std::back_inserter(assetTextureNamesVec));

    return OnLoadAssetTextureInternal(assetTextureNamesVec, tag, resultWhen);
}

Render::TextureId TextureResources::OnLoadAssetTextureInternal(const std::vector<std::string>& assetTextureNames, const std::string& tag, ResultWhen resultWhen)
{
    const auto assetHash = GetAssetHash(assetTextureNames);

    //
    // If the asset already has a record, return it
    //
    {
        std::lock_guard<std::mutex> assetsLock(m_assetsMutex);

        const auto it = m_assetToTexture.find(assetHash);
        if (it != m_assetToTexture.cend())
        {
            return it->second;
        }

        //
        // If we're here, we need to load the asset, so create a record for it
        // to prevent subsequent calls for the same asset from doing any work
        //
        m_assetToTexture.insert({assetHash, Render::TextureId()});
    }

    //
    // Otherwise, we don't have this asset loaded already, so load the texture(s) data from disk
    //
    std::expected<TextureData, bool> textureDataExpect;

    if (assetTextureNames.size() == 1)
    {
        textureDataExpect = m_assets->ReadTextureBlocking(assetTextureNames[0]);
    }
    else if (assetTextureNames.size() == 6)
    {
        std::array<std::string, 6> assetTextureNamesArray;
        for (unsigned int x = 0; x < 6; ++x) { assetTextureNamesArray[x] = assetTextureNames[x]; }

        textureDataExpect = m_assets->ReadCubeTextureBlocking(assetTextureNamesArray);
    }

    if (!textureDataExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::OnLoadAssetTextureInternal: Failed to read texture: {}", tag);
        return Render::INVALID_ID;
    }

    //
    // Create and record the texture
    //
    const auto textureId = m_renderer->GetIds()->textureIds.GetId();
    const auto texture = ToRenderTexture(textureId, *textureDataExpect, tag);

    Render::TextureView textureView;
    if (texture.numLayers == 1)
    {
        textureView = Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT, Render::TextureView::Aspect::ASPECT_COLOR_BIT);
    }
    else
    {
        textureView = Render::TextureView::ViewAsCube(Render::TextureView::DEFAULT, Render::TextureView::Aspect::ASPECT_COLOR_BIT);
    }

    {
        std::lock_guard<std::mutex> texturesLock(m_texturesMutex);
        std::lock_guard<std::mutex> assetsLock(m_assetsMutex);

        m_assetToTexture[assetHash] = textureId;
        m_textureToAsset[textureId] = assetHash;
        m_textures.insert({textureId, RegisteredTexture{texture}});
    }

    const auto textureSampler = Render::TextureSampler(Render::CLAMP_ADDRESS_MODE);

    //
    // Send the texture to the renderer
    //
    const bool generateMipMaps = texture.numLayers == 1;

    std::future<bool> transferFuture = m_renderer->CreateTexture(texture, textureView, textureSampler, generateMipMaps);

    if (resultWhen == ResultWhen::FullyLoaded && !transferFuture.get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::OnLoadAssetTextureInternal: Renderer failed to create texture: {}", tag);
        DestroyTexture(textureId);
        return Render::INVALID_ID;
    }

    return textureId;
}

std::expected<TextRender, bool> TextureResources::OnRenderText(const std::string& text, const Platform::TextProperties& properties, ResultWhen resultWhen)
{
    const std::string tag = "TextRender";

    //
    // Attempt to lazily/on-demand load the font if it hadn't been loaded already
    //
    if (!m_text->IsFontLoaded(properties.fontFileName, properties.fontSize))
    {
        m_logger->Log(Common::LogLevel::Info,
            "TextureResources::OnRenderText: Attempting to lazily load font: {}x{}", properties.fontFileName, properties.fontSize);

        if (!m_text->LoadFontBlocking(properties.fontFileName, properties.fontSize))
        {
            m_logger->Log(Common::LogLevel::Error,
                "TextureResources::OnRenderText: Failed to lazily load font: {}x{}", properties.fontFileName, properties.fontSize);
            return std::unexpected(false);
        }
    }

    //
    // Have the platform render the text to an image
    //
    const auto renderedTextExpect = m_text->RenderText(text, properties);
    if (!renderedTextExpect) { return std::unexpected(false); }

    //
    // Create and record the texture
    //
    const auto textureId = m_renderer->GetIds()->textureIds.GetId();
    const auto texture = Render::Texture::FromImageData(textureId, Render::TextureUsage::ImageMaterial, 1, renderedTextExpect->imageData, tag);
    const auto textureView = Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT, Render::TextureView::Aspect::ASPECT_COLOR_BIT);
    const auto textureSampler = Render::TextureSampler(Render::CLAMP_ADDRESS_MODE);

    {
        std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

        m_textures.insert({textureId, RegisteredTexture{texture}});
    }

    //
    // Send the texture to the renderer
    //
    std::future<bool> transferFuture = m_renderer->CreateTexture(texture, textureView, textureSampler, false);

    if (resultWhen == ResultWhen::FullyLoaded && !transferFuture.get())
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::OnRenderText: Renderer failed to create texture: {}", tag);
        DestroyTexture(textureId);
        return std::unexpected(false);
    }

    TextRender textRender{};
    textRender.textureId = textureId;
    textRender.textPixelWidth = renderedTextExpect->textPixelWidth;
    textRender.textPixelHeight = renderedTextExpect->textPixelHeight;

    return textRender;
}

std::optional<Render::TextureId> TextureResources::GetAssetTextureId(const std::string& assetTextureName) const
{
    const auto assetHash = GetAssetHash({assetTextureName});

    std::lock_guard<std::mutex> assetsLock(m_assetsMutex);

    const auto it = m_assetToTexture.find(assetHash);
    if (it == m_assetToTexture.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<Render::Texture> TextureResources::GetLoadedTextureData(const Render::TextureId& textureId) const
{
    std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

    const auto it = m_textures.find(textureId);
    if (it != m_textures.cend())
    {
        return it->second.texture;
    }

    return std::nullopt;
}

void TextureResources::DestroyTexture(const Render::TextureId& textureId)
{
    if (!textureId.IsValid()) { return; }

    m_logger->Log(Common::LogLevel::Debug, "TextureResources::DestroyTexture: Destroying texture, id: {}", textureId.id);

    std::lock_guard<std::mutex> assetsLock(m_assetsMutex);
    std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

    //
    // Destroy any asset tracking data
    //
    const auto assetsIt = m_textureToAsset.find(textureId);
    if (assetsIt != m_textureToAsset.cend())
    {
        m_assetToTexture.erase(assetsIt->second);
        m_textureToAsset.erase(assetsIt);
    }

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
    m_logger->Log(Common::LogLevel::Info, "TextureResources::DestroyAll: Destroying all textures");

    while (!m_textures.empty())
    {
        DestroyTexture(m_textures.cbegin()->first);
    }
}

Render::Texture TextureResources::ToRenderTexture(Render::TextureId textureId, const TextureData& textureData, const std::string& tag)
{
    const auto imageData = TextureDataToImageData(textureData);

    Render::TextureUsage textureUsage = Render::TextureUsage::ImageMaterial;
    if (imageData->GetNumLayers() == 6)
    {
        textureUsage = Render::TextureUsage::ImageCubeMaterial;
    }

    return Render::Texture::FromImageData(textureId, textureUsage, imageData->GetNumLayers(), imageData, tag);
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

std::size_t TextureResources::GetAssetHash(const std::vector<std::string>& fileNames)
{
    std::stringstream ss;

    for (const auto& fileName : fileNames)
    {
        ss << fileName;
    }

    return std::hash<std::string>{}(ss.str());
}

}
