/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H

#include <Accela/Engine/Scene/IFontResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

namespace Accela::Platform
{
    class IText;
}

namespace Accela::Engine
{
    class FontResources : public IFontResources
    {
        public:

            FontResources(Common::ILogger::Ptr logger,
                          std::shared_ptr<Platform::IText> m_text,
                          std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IFontResources
            //
            [[nodiscard]] std::future<bool> LoadFont(const std::string& fontFileName, uint8_t fontSize) override;
            [[nodiscard]] std::future<bool> LoadFont(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize) override;
            [[nodiscard]] bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) override;
            void DestroyFont(const std::string& fontFileName) override;
            void DestroyFont(const std::string& fontFileName, uint8_t fontSize) override;
            void DestroyAll() override;

        private:

            [[nodiscard]] bool OnLoadFont(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize);

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<Platform::IText> m_text;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_FONTRESOURCES_H
