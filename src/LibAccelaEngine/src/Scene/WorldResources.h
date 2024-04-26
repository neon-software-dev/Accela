/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H

#include "HeightMapData.h"

#include "../ForwardDeclares.h"

#include "../Model/RegisteredModel.h"

#include <Accela/Engine/Scene/IWorldResources.h>

#include <Accela/Render/Texture/Texture.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <expected>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace Accela::Engine
{
    class IEngineAssets;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Platform
{
    class IFiles;
    class IText;
}

namespace Accela::Engine
{
    class WorldResources : public IWorldResources
    {
        public:

            WorldResources(Common::ILogger::Ptr logger,
                           std::shared_ptr<Render::IRenderer> renderer,
                           std::shared_ptr<Platform::IFiles> files,
                           std::shared_ptr<IEngineAssets> assets,
                           std::shared_ptr<Platform::IText> text,
                           AudioManagerPtr audioManager);

            [[nodiscard]] ITextureResources::Ptr Textures() const override;
            [[nodiscard]] IMeshResources::Ptr Meshes() const override;

            // Materials
            [[nodiscard]] Render::MaterialId RegisterObjectMaterial(const Render::ObjectMaterialProperties& properties,
                                                                    const std::string& tag) override;
            void DestroyMaterial(Render::MaterialId materialId) override;

            // Audio
            [[nodiscard]] bool RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData) override;
            void DestroyAudio(const std::string& name) override;

            // Fonts
            [[nodiscard]] bool LoadFontBlocking(const std::string& fontFileName, uint8_t fontSize) override;
            [[nodiscard]] bool LoadFontBlocking(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize) override;
            [[nodiscard]] bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) override;

            // Models
            [[nodiscard]] bool RegisterModel(const std::string& modelName, const Model::Ptr& model) override;
            [[nodiscard]] std::optional<RegisteredModel> GetRegisteredModel(const std::string& modelName) const;

        private:

            std::expected<Render::MaterialId, bool> LoadModelMeshMaterial(RegisteredModel& registeredModel,
                                                                         const std::string& modelName,
                                                                         const ModelMaterial& material);

            std::expected<Render::TextureId, bool> LoadModelMaterialTexture(RegisteredModel& registeredModel,
                                                                           const std::string& modelName,
                                                                           const ModelTexture& modelTexture);

            static HeightMapData::Ptr GenerateHeightMapData(const Common::ImageData::Ptr& heightMapImage,
                                                            const Render::USize& heightMapDataSize,
                                                            const Render::USize& meshSize_worldSpace,
                                                            const float& displacementFactor);

            Render::Mesh::Ptr GenerateHeightMapMesh(const HeightMapData& heightMapData,
                                                    const Render::USize& meshSize_worldSpace,
                                                    const std::string& tag) const;

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Platform::IText> m_text;
            AudioManagerPtr m_audioManager;

            ITextureResources::Ptr m_textures;
            IMeshResources::Ptr m_meshes;

            // Models registered via RegisterModel calls
            std::unordered_map<std::string, RegisteredModel> m_registeredModels;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_WORLDRESOURCES_H
