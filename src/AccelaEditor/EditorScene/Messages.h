/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_MESSAGES_H
#define ACCELAEDITOR_EDITORSCENE_MESSAGES_H

#include <Accela/Engine/Common.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela
{
    struct SceneQuitCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<SceneQuitCommand>;
        static constexpr auto TYPE = "SceneQuitCommand";
        SceneQuitCommand() : Common::Message(TYPE) { }
    };

    struct LoadPackageResourcesCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<LoadPackageResourcesCommand>;
        static constexpr auto TYPE = "LoadPackageResourcesCommand";
        explicit LoadPackageResourcesCommand(Engine::PackageName _packageName)
            : Common::ResultMessage<bool>(TYPE)
            , packageName(std::move(_packageName))
        { }
        Engine::PackageName packageName;
    };

    struct DestroySceneResourcesCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<DestroySceneResourcesCommand>;
        static constexpr auto TYPE = "DestroySceneResourcesCommand";
        DestroySceneResourcesCommand() : Common::ResultMessage<bool>(TYPE) { }
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_MESSAGES_H
