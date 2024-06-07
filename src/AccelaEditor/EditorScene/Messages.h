/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_MESSAGES_H
#define ACCELAEDITOR_EDITORSCENE_MESSAGES_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Package/Component.h>

#include <Accela/Common/Thread/ResultMessage.h>

#include <vector>

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

    struct CreateEntityCommand : public Common::ResultMessage<Engine::EntityId>
    {
        using Ptr = std::shared_ptr<CreateEntityCommand>;
        static constexpr auto TYPE = "CreateEntityCommand";
        CreateEntityCommand() : Common::ResultMessage<Engine::EntityId>(TYPE) { }
    };

    struct DestroyEntityCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<DestroyEntityCommand>;
        static constexpr auto TYPE = "DestroyEntityCommand";
        explicit DestroyEntityCommand(const Engine::EntityId& _eid)
            : Common::ResultMessage<bool>(TYPE)
            , eid(_eid)
            { }

        Engine::EntityId eid;
    };

    struct DestroyAllEntitiesCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<DestroyAllEntitiesCommand>;
        static constexpr auto TYPE = "DestroyAllEntitiesCommand";
        explicit DestroyAllEntitiesCommand()
            : Common::ResultMessage<bool>(TYPE) { }
    };

    struct SetEntityComponentCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<SetEntityComponentCommand>;
        static constexpr auto TYPE = "SetEntityComponentCommand";
        explicit SetEntityComponentCommand(Engine::EntityId _eid, Engine::Component::Ptr _component)
            : Common::ResultMessage<bool>(TYPE)
            , eid(_eid)
            , component(std::move(_component))
          { }

        Engine::EntityId eid;
        Engine::Component::Ptr component;
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_MESSAGES_H
