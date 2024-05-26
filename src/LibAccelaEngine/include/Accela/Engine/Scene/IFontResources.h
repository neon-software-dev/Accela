/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <memory>
#include <future>

namespace Accela::Engine
{
    /**
     * Encapsulates font resource operations
     */
    class IFontResources
    {
        public:

            using Ptr = std::shared_ptr<IFontResources>;

        public:

            virtual ~IFontResources() = default;

            /**
             * Loads a single size of a font resource from a package.
             *
             * @param resource Identifies the font resource
             * @param fontSize The specific font size to be loaded
             *
             * @return A future containing whether the operation was successful
             */
            [[nodiscard]] virtual std::future<bool> LoadFont(const PackageResourceIdentifier& resource, uint8_t fontSize) = 0;

            /**
             * Loads a range of sizes of a font resource from a package.
             *
             * @param resource Identifies the font resource
             * @param startFontSize The inclusive starting font size to load
             * @param endFontSize The inclusive ending font size to load
             *
             * @return A future containing whether all font sizes loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadFont(const PackageResourceIdentifier& resource,
                                                             uint8_t startFontSize,
                                                             uint8_t endFontSize) = 0;

            /**
             * Loads a range of font sizes for every font within a package.
             *
             * @param packageName Identifies the package
             * @param startFontSize The inclusive starting font size to load
             * @param endFontSize The inclusive ending font size to load
             *
             * @return A future containing whether all fonts/sizes loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllFonts(const PackageName& packageName,
                                                                 uint8_t startFontSize,
                                                                 uint8_t endFontSize) = 0;

            /**
             * Loads a range of font sizes for every font within every registered package.
             *
             * @param startFontSize The inclusive starting font size to load
             * @param endFontSize The inclusive ending font size to load
             *
             * @return A future containing whether all fonts/sizes loaded successfully
             */
            [[nodiscard]] virtual std::future<bool> LoadAllFonts(uint8_t startFontSize, uint8_t endFontSize) = 0;

            /**
             * Query for whether a specific font size of a specific font resource is loaded.
             *
             * @return Whether the font + size is loaded
             */
            [[nodiscard]] virtual bool IsFontLoaded(const ResourceIdentifier& resource, uint8_t fontSize) = 0;

            /**
             * Destroy all sizes of a previously loaded font resource
             *
             * @param resource Identifies the font resource
             */
            virtual void DestroyFont(const ResourceIdentifier& resource) = 0;

            /**
            * Destroy a particular size of a previously loaded font resource
            *
            * @param resource Identifies the font resource
            * @param fontSize The size of the font to destroy
            */
            virtual void DestroyFont(const ResourceIdentifier& resource, uint8_t fontSize) = 0;

            /**
             * Destroy all previously loaded font resources
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H
