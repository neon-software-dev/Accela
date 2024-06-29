/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H

#include "../ForwardDeclares.h"

#include <Accela/Engine/Scene/IFontResources.h>

#include "Accela/Platform/Package/PackageSource.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

namespace Accela::Platform
{
    class IText;
}

namespace Accela::Engine
{
    //
    // Note: Unlike other resource classes, the fonts are not identified by package name once they're
    // loaded; only by font filename. Multiple packages loading the same font will cause the font to
    // only be loaded once, and allow for entities to address fonts without having to specify which
    // package the font belongs to.
    //
    class FontResources : public IFontResources
    {
        public:

            FontResources(Common::ILogger::Ptr logger,
                          PackageResourcesPtr packages,
                          std::shared_ptr<Platform::IText> text,
                          std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IFontResources
            //
            [[nodiscard]] std::future<bool> LoadFont(const PackageResourceIdentifier& resource, uint8_t fontSize) override;
            [[nodiscard]] std::future<bool> LoadFont(const PackageResourceIdentifier& resource, uint8_t startFontSize, uint8_t endFontSize) override;
            [[nodiscard]] std::future<bool> LoadAllFonts(const PackageName& packageName, uint8_t startFontSize, uint8_t endFontSize) override;
            [[nodiscard]] std::future<bool> LoadAllFonts(uint8_t startFontSize, uint8_t endFontSize) override;
            [[nodiscard]] bool IsFontLoaded(const ResourceIdentifier& resource, uint8_t fontSize) override;
            void DestroyFont(const ResourceIdentifier& resource) override;
            void DestroyFont(const ResourceIdentifier& resource, uint8_t fontSize) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnLoadFont(const PackageResourceIdentifier& resource, uint8_t startFontSize, uint8_t endFontSize);
            [[nodiscard]] bool OnLoadAllFonts(const PackageName& packageName, uint8_t startFontSize, uint8_t endFontSize);
            [[nodiscard]] bool OnLoadAllFonts(uint8_t startFontSize, uint8_t endFontSize);

            [[nodiscard]] bool LoadPackageFont(const Platform::PackageSource::Ptr& package,
                                               const PackageResourceIdentifier& resource,
                                               uint8_t startFontSize,
                                               uint8_t endFontSize);

        private:

            Common::ILogger::Ptr m_logger;
            PackageResourcesPtr m_packages;
            std::shared_ptr<Platform::IText> m_text;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H
