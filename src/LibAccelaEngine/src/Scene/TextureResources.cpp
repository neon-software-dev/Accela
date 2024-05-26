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
                                   IPackageResourcesPtr packages,
                                   std::shared_ptr<Render::IRenderer> renderer,
                                   std::shared_ptr<Platform::IText> text,
                                   std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_packages(std::move(packages))
    , m_renderer(std::move(renderer))
    , m_text(std::move(text))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::TextureId> TextureResources::LoadTexture(const PackageResourceIdentifier& resource, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadTexture(resource, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadTexture(const PackageResourceIdentifier& resource, ResultWhen resultWhen)
{
    return LoadPackageTexture({resource}, resource.GetUniqueName(), resultWhen);
}

std::future<Render::TextureId> TextureResources::LoadCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources, const std::string& tag, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadCubeTexture(resources, tag, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources, const std::string& tag, ResultWhen resultWhen)
{
    std::vector<PackageResourceIdentifier> resourcesVec;
    std::ranges::copy(resources, std::back_inserter(resourcesVec));

    return LoadPackageTexture(resourcesVec, tag, resultWhen);
}

std::future<Render::TextureId> TextureResources::LoadTexture(const CustomResourceIdentifier& resource, const Common::ImageData::Ptr& imageData, ResultWhen resultWhen)
{
    auto message = std::make_shared<TextureResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<TextureResultMessage>(_message)->SetResult(
            OnLoadTexture(resource, imageData, resultWhen)
        );
    });

    return messageFuture;
}

Render::TextureId TextureResources::OnLoadTexture(const CustomResourceIdentifier& resource,
                                                  const Common::ImageData::Ptr& imageData,
                                                  ResultWhen resultWhen)
{
    return LoadCustomTexture(resource, imageData, resultWhen);
}

std::future<bool> TextureResources::LoadAllTextures(const PackageName& packageName, ResultWhen resultWhen)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllTextures(packageName, resultWhen)
        );
    });

    return messageFuture;
}

bool TextureResources::OnLoadAllTextures(const PackageName& packageName, ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading all texture resources from package: {}", packageName.name);

    const auto package = m_packages->GetPackage(packageName);
    if (!package)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TextureResources::OnLoadAllTextures: No such package: {}", packageName.name);
        return false;
    }

    const auto textureFileNames = (*package)->GetTextureFileNames();

    bool allSuccess = true;

    for (const auto& textureFileName : textureFileNames)
    {
        if (!OnLoadTexture(PackageResourceIdentifier(packageName, textureFileName), resultWhen).IsValid())
        {
            allSuccess = false;
        }
    }

    return allSuccess;
}

std::future<bool> TextureResources::LoadAllTextures(ResultWhen resultWhen)
{
    auto message = std::make_shared<Common::BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<Common::BoolResultMessage>(_message)->SetResult(
            OnLoadAllTextures(resultWhen)
        );
    });

    return messageFuture;
}

bool TextureResources::OnLoadAllTextures(ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading all textures");

    const auto packages = m_packages->GetAllPackages();

    bool allSuccessful = true;

    for (const auto& package : packages)
    {
        allSuccessful = allSuccessful && OnLoadAllTextures(PackageName(package->GetPackageName()), resultWhen);
    }

    return allSuccessful;
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

Render::TextureId TextureResources::LoadCustomTexture(const CustomResourceIdentifier& resource, const Common::ImageData::Ptr& imageData, ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading custom texture resource: {}", resource.GetUniqueName());

    const auto resourcesHash = GetResourcesHash(std::vector<ResourceIdentifier>{resource});

    return LoadTexture(TextureData(imageData), resourcesHash, resource.GetUniqueName(), resultWhen);
}

Render::TextureId TextureResources::LoadPackageTexture(const std::vector<PackageResourceIdentifier>& resources, const std::string& tag, ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Loading package texture resource: {}", tag);

    const auto resourcesHash = GetResourcesHash(resources);

    //
    // Fetch the package for each resource
    //
    std::vector<Platform::Package::Ptr> packages;

    for (const auto& resource : resources)
    {
        const auto package = m_packages->GetPackage(*resource.GetPackageName());
        if (!package)
        {
            m_logger->Log(Common::LogLevel::Error,
              "TextureResources::LoadPackageTexture: No such package: {}", resource.GetPackageName()->name);
            return false;
        }

        packages.push_back(*package);
    }

    //
    // Load the texture data from the packages
    //
    TextureData textureData{};

    for (unsigned int x = 0; x < resources.size(); ++x)
    {
        const auto textureDataExpect = packages.at(x)->GetTextureData(resources.at(x).GetResourceName());
        if (!textureDataExpect)
        {
            m_logger->Log(Common::LogLevel::Error,
              "TextureResources::LoadPackageTexture: Failed to read texture: {}", resources.at(x).GetUniqueName());
            return Render::INVALID_ID;
        }
        textureData.textureImages.push_back(*textureDataExpect);
    }

    //
    // Create and record the texture
    //
    return LoadTexture(textureData, resourcesHash, tag, resultWhen);
}

Render::TextureId TextureResources::LoadTexture(const TextureData& textureData,
                                                const std::size_t& resourcesHash,
                                                const std::string& tag,
                                                ResultWhen resultWhen)
{
    //
    // Check if the resource is already loaded
    //
    {
        std::lock_guard<std::mutex> resourcesLock(m_resourcesMutex);

        const auto it = m_resourceToTexture.find(resourcesHash);
        if (it != m_resourceToTexture.cend())
        {
            m_logger->Log(Common::LogLevel::Warning,
              "TextureResources::LoadTexture: Texture already loaded, ignoring: {}", resourcesHash);
            return true;
        }

        //
        // If not, we need to load the resource, so create a record for it
        // to prevent subsequent calls for the same resource from doing any work
        //
        m_resourceToTexture.insert({resourcesHash, Render::TextureId()});
    }

    //
    // Create and record the texture
    //
    const auto textureId = m_renderer->GetIds()->textureIds.GetId();
    const auto texture = ToRenderTexture(textureId, textureData, tag);

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
        std::lock_guard<std::mutex> resourcesLock(m_resourcesMutex);

        m_resourceToTexture[resourcesHash] = textureId;
        m_textureToResource[textureId] = resourcesHash;
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
          "TextureResources::LoadTexture: Renderer failed to create texture: {}", tag);
        DestroyTexture(textureId);
        return Render::INVALID_ID;
    }

    return textureId;
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

std::optional<Render::TextureId> TextureResources::GetTextureId(const ResourceIdentifier& resource) const
{
    const auto resourceHash = GetResourcesHash(std::vector<ResourceIdentifier>{resource});

    std::lock_guard<std::mutex> resourcesLock(m_resourcesMutex);

    const auto it = m_resourceToTexture.find(resourceHash);
    if (it == m_resourceToTexture.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<Render::Texture> TextureResources::GetLoadedTextureData(const ResourceIdentifier& resource) const
{
    const auto resourceHash = GetResourcesHash(std::vector<ResourceIdentifier>{resource});

    std::lock_guard<std::mutex> resourcesLock(m_resourcesMutex);

    const auto it = m_resourceToTexture.find(resourceHash);
    if (it == m_resourceToTexture.cend())
    {
        return std::nullopt;
    }

    return GetLoadedTextureData(it->second);
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

void TextureResources::DestroyTexture(const ResourceIdentifier& resource)
{
    const auto textureId = GetTextureId(resource);
    if (!textureId)
    {
        return;
    }

    DestroyTexture(*textureId);
}

void TextureResources::DestroyTexture(const Render::TextureId& textureId)
{
    if (!textureId.IsValid()) { return; }

    std::lock_guard<std::mutex> resourcesLock(m_resourcesMutex);
    std::lock_guard<std::mutex> texturesLock(m_texturesMutex);

    const auto it = m_textures.find(textureId);
    if (it == m_textures.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "TextureResources: Destroying texture resource: {}", it->second.texture.tag);

    //
    // Destroy any resource tracking data
    //
    const auto resourcesIt = m_textureToResource.find(textureId);
    if (resourcesIt != m_textureToResource.cend())
    {
        m_resourceToTexture.erase(resourcesIt->second);
        m_textureToResource.erase(resourcesIt);
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
    m_logger->Log(Common::LogLevel::Info, "TextureResources: Destroying all texture resources");

    while (!m_textures.empty())
    {
        DestroyTexture(m_textures.cbegin()->first);
    }
}

}
