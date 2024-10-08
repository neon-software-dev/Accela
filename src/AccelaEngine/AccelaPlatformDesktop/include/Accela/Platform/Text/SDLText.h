/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H
#define LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H

#include <Accela/Platform/Text/IText.h>
#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>

#include <utility>
#include <cstdint>
#include <unordered_map>
#include <mutex>

struct _TTF_Font;

namespace Accela::Platform
{
    class ACCELA_PUBLIC SDLText : public IText
    {
        public:

            explicit SDLText(Common::ILogger::Ptr logger);

            void Destroy() override;

            bool LoadFontBlocking(const std::string& fontFileName, const std::vector<std::byte>& fontData, uint8_t fontSize) override;
            bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) override;
            void UnloadFont(const std::string& fontFileName) override;
            void UnloadFont(const std::string& fontFileName, uint8_t fontSize) override;
            void UnloadAllFonts() override;

            std::expected<RenderedText, bool> RenderText(const std::string& text, const TextProperties& properties) const override;

        private:

            struct LoadedFont
            {
                std::vector<std::byte> fontData;
                _TTF_Font* pFont;
            };

        private:

            _TTF_Font* GetLoadedFont(const std::string& fontFileName, uint8_t fontSize) const;

        private:

            Common::ILogger::Ptr m_logger;

            // Font file name -> [{Font size -> Font}]
            std::unordered_map<std::string, std::unordered_map<uint8_t, std::shared_ptr<LoadedFont>>> m_fonts;
            mutable std::recursive_mutex m_fontsMutex;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H
