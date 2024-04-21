/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H
#define LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H

#include <Accela/Platform/Text/IText.h>
#include <Accela/Platform/File/IFiles.h>

#include <Accela/Common/Log/ILogger.h>

#include <SDL2/SDL_ttf.h>

#include <utility>
#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace Accela::Platform
{
    class SDLText : public IText
    {
        public:

            SDLText(Common::ILogger::Ptr logger, IFiles::Ptr files);

            void Destroy() override;

            bool LoadFontBlocking(const std::string& fontFileName, uint8_t fontSize) override;
            bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) override;
            void UnloadFont(const std::string& fontFileName) override;
            void UnloadFont(const std::string& fontFileName, uint8_t fontSize) override;

            std::expected<RenderedText, bool> RenderText(const std::string& text, const TextProperties& properties) const override;

        private:

            TTF_Font* GetLoadedFont(const std::string& fontFileName, uint8_t fontSize) const;

        private:

            Common::ILogger::Ptr m_logger;
            IFiles::Ptr m_files;

            // Font file name -> [{Font size -> Font}]
            std::unordered_map<std::string, std::unordered_map<uint8_t, TTF_Font*>> m_fonts;
            mutable std::recursive_mutex m_fontsMutex;
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_TEXT_SDLTEXT_H
