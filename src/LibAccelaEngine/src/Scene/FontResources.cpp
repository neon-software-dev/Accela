/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FontResources.h"

#include <Accela/Platform/Text/IText.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

struct BoolResultMessage : public Common::ResultMessage<bool>
{
    BoolResultMessage()
        : Common::ResultMessage<bool>("BoolResultMessage")
    { }
};

FontResources::FontResources(Common::ILogger::Ptr logger,
                             std::shared_ptr<Platform::IText> text,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
     : m_logger(std::move(logger))
     , m_text(std::move(text))
     , m_threadPool(std::move(threadPool))
{

}

std::future<bool> FontResources::LoadFont(const std::string& fontFileName, uint8_t fontSize)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<BoolResultMessage>(_message)->SetResult(
            OnLoadFont(fontFileName, fontSize, fontSize)
        );
    });

    return messageFuture;
}

std::future<bool> FontResources::LoadFont(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<BoolResultMessage>(_message)->SetResult(
            OnLoadFont(fontFileName, startFontSize, endFontSize)
        );
    });

    return messageFuture;
}

bool FontResources::IsFontLoaded(const std::string& fontFileName, uint8_t fontSize)
{
    return m_text->IsFontLoaded(fontFileName, fontSize);
}

void FontResources::DestroyFont(const std::string& fontFileName)
{
    m_text->UnloadFont(fontFileName);
}

void FontResources::DestroyFont(const std::string& fontFileName, uint8_t fontSize)
{
    m_logger->Log(Common::LogLevel::Info, "FontResources::DestroyFont: Destroying font: {}x{}", fontFileName, fontSize);

    m_text->UnloadFont(fontFileName, fontSize);
}

void FontResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "FontResources::DestroyAll: Destroying all fonts");

    m_text->UnloadAllFonts();
}

bool FontResources::OnLoadFont(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize)
{
    bool allSuccessful = true;

    for (uint8_t fontSize = startFontSize; fontSize <= endFontSize; ++fontSize)
    {
        if (!m_text->LoadFontBlocking(fontFileName, fontSize)) { allSuccessful = false; }
    }

    return allSuccessful;
}

}
