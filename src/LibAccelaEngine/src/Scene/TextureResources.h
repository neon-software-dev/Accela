#ifndef LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H

#include "../ForwardDeclares.h"
#include "../Texture/RegisteredTexture.h"

#include <Accela/Engine/Scene/ITextureResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <expected>
#include <memory>
#include <future>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <sstream>

namespace Accela::Platform
{
    class IText;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class TextureResources : public ITextureResources
    {
        public:

            TextureResources(Common::ILogger::Ptr logger,
                             IPackageResourcesPtr packages,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<Platform::IText> text,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // ITextureResources
            //
            [[nodiscard]] std::future<Render::TextureId> LoadTexture(const PackageResourceIdentifier& resource, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources, const std::string& tag, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<Render::TextureId> LoadTexture(const CustomResourceIdentifier& resource,
                                                        const Common::ImageData::Ptr& imageData,
                                                        ResultWhen resultWhen) override;
            [[nodiscard]] std::future<bool> LoadAllTextures(const PackageName& packageName, ResultWhen resultWhen) override;
            [[nodiscard]] std::future<bool> LoadAllTextures(ResultWhen resultWhen) override;
            [[nodiscard]] std::future<std::expected<TextRender, bool>> RenderText(const std::string& text,
                                                                                  const Platform::TextProperties& properties,
                                                                                  ResultWhen resultWhen) override;
            [[nodiscard]] std::optional<Render::TextureId> GetTextureId(const ResourceIdentifier& resource) const override;
            [[nodiscard]] std::optional<Render::Texture> GetLoadedTextureData(const ResourceIdentifier& resource) const override;
            [[nodiscard]] std::optional<Render::Texture> GetLoadedTextureData(const Render::TextureId& textureId) const override;
            void DestroyTexture(const ResourceIdentifier& resource) override;
            void DestroyTexture(const Render::TextureId& textureId) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] Render::TextureId OnLoadTexture(const PackageResourceIdentifier& resource, ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadCubeTexture(const std::array<PackageResourceIdentifier, 6>& resources,
                                                              const std::string& tag,
                                                              ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId OnLoadTexture(const CustomResourceIdentifier& resource,
                                                          const Common::ImageData::Ptr& imageData,
                                                          ResultWhen resultWhen);
            [[nodiscard]] bool OnLoadAllTextures(const PackageName& packageName, ResultWhen resultWhen);
            [[nodiscard]] bool OnLoadAllTextures(ResultWhen resultWhen);
            [[nodiscard]] std::expected<TextRender, bool> OnRenderText(const std::string& text,
                                                                       const Platform::TextProperties& properties,
                                                                       ResultWhen resultWhen);

            [[nodiscard]] Render::TextureId LoadCustomTexture(const CustomResourceIdentifier& resource,
                                                              const Common::ImageData::Ptr& imageData,
                                                              ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId LoadPackageTexture(const std::vector<PackageResourceIdentifier>& resources,
                                                               const std::string& tag,
                                                               ResultWhen resultWhen);
            [[nodiscard]] Render::TextureId LoadTexture(const TextureData& textureData,
                                                        const std::size_t& resourcesHash,
                                                        const std::string& tag,
                                                        ResultWhen resultWhen);

            [[nodiscard]] static Render::Texture ToRenderTexture(Render::TextureId textureId,
                                                                 const TextureData& textureData,
                                                                 const std::string& tag);
            [[nodiscard]] static Common::ImageData::Ptr TextureDataToImageData(const TextureData& textureData);

            template <typename T>
            [[nodiscard]] static std::size_t GetResourcesHash(const std::vector<T>& resources)
            {
                std::stringstream ss;
                for (const auto& resource : resources) { ss << resource.GetUniqueName(); }
                return std::hash<std::string>{}(ss.str());
            }

        private:

            Common::ILogger::Ptr m_logger;
            IPackageResourcesPtr m_packages;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IText> m_text;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_resourcesMutex;
            std::unordered_map<std::size_t, Render::TextureId> m_resourceToTexture;
            std::unordered_map<Render::TextureId, std::size_t> m_textureToResource;

            mutable std::mutex m_texturesMutex;
            std::unordered_map<Render::TextureId, RegisteredTexture> m_textures;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_TEXTURERESOURCES_H
