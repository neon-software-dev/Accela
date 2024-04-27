/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H

#include <string>
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
             * Loads a single size of a font from the assets fonts directory
             *
             * @param fontFileName The filename of the font to be loaded
             * @param fontSize The font size to be loaded
             *
             * @return A future with the result of the operation
             */
            [[nodiscard]] virtual std::future<bool> LoadFont(const std::string& fontFileName, uint8_t fontSize) = 0;

            /**
             * Loads a range of sizes of a font from the assets fonts directory
             *
             * @param fontFileName The filename of the font to be loaded
             * @param startFontSize The inclusive starting font size to load
             * @param endFontSize The inclusive ending font size to load
             *
             * @return A future with the result of the operation
             */
            [[nodiscard]] virtual std::future<bool> LoadFont(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize) = 0;

            /**
             * @return Whether the specific font file and font size is currently loaded
             */
            [[nodiscard]] virtual bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) = 0;

            /**
             * Destroy all sizes of a previously loaded font
             *
             * @param fontFileName The name of the font to destroy
             */
            virtual void DestroyFont(const std::string& fontFileName) = 0;

            /**
            * Destroy a particular size of a previously loaded font
            *
            * @param fontFileName The name of the font to destroy
            * @param fontSize The size of the font to destroy
            */
            virtual void DestroyFont(const std::string& fontFileName, uint8_t fontSize) = 0;

            /**
             * Destroy all previously loaded font
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IFONTRESOURCES_H
