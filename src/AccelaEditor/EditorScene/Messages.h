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
#include <unordered_set>

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

    struct RemoveEntityComponentCommand : public Common::ResultMessage<bool>
    {
        using Ptr = std::shared_ptr<RemoveEntityComponentCommand>;
        static constexpr auto TYPE = "RemoveEntityComponentCommand";
        explicit RemoveEntityComponentCommand(Engine::EntityId _eid, Engine::Component::Type _type)
            : Common::ResultMessage<bool>(TYPE)
            , eid(_eid)
            , type(_type)
        { }

        Engine::EntityId eid;
        Engine::Component::Type type;
    };

    struct RotateCameraCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<RotateCameraCommand>;
        static constexpr auto TYPE = "RotateCameraCommand";
        RotateCameraCommand(int _xRot, int _yRot)
            : Common::Message(TYPE)
            , xRot(_xRot)
            , yRot(_yRot)
        { }

        int xRot;
        int yRot;
    };

    struct PanCameraCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<PanCameraCommand>;
        static constexpr auto TYPE = "PanCameraCommand";
        PanCameraCommand(int _xPan, int _yPan)
            : Common::Message(TYPE)
            , xPan(_xPan)
            , yPan(_yPan)
        { }

        int xPan;
        int yPan;
    };

    struct ScaleCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<ScaleCommand>;
        static constexpr auto TYPE = "ScaleCommand";
        explicit ScaleCommand(float _scaleDeltaDegrees)
            : Common::Message(TYPE)
            , scaleDeltaDegrees(_scaleDeltaDegrees)
        { }

        float scaleDeltaDegrees;
    };

    struct EntityClicked : public Common::Message
    {
        using Ptr = std::shared_ptr<EntityClicked>;
        static constexpr auto TYPE = "EntityClicked";
        EntityClicked(Engine::EntityId _eid, bool _requestingMultipleSelect)
            : Common::Message(TYPE)
            , eid(_eid)
            , requestingMultipleSelect(_requestingMultipleSelect)
        { }

        Engine::EntityId eid;
        bool requestingMultipleSelect;
    };

    struct NothingClicked : public Common::Message
    {
        using Ptr = std::shared_ptr<NothingClicked>;
        static constexpr auto TYPE = "NothingClicked";
        NothingClicked()
            : Common::Message(TYPE)
        { }
    };

    struct SetEntitiesHighlightedCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<SetEntitiesHighlightedCommand>;
        static constexpr auto TYPE = "SetEntitiesHighlightedCommand";
        explicit SetEntitiesHighlightedCommand(std::unordered_set<Engine::EntityId> _eids)
            : Common::Message(TYPE)
            , eids(std::move(_eids))
        { }

        std::unordered_set<Engine::EntityId> eids;
    };

    struct ResetCameraCommand : public Common::Message
    {
        using Ptr = std::shared_ptr<ResetCameraCommand>;
        static constexpr auto TYPE = "ResetCameraCommand";
        ResetCameraCommand() : Common::Message(TYPE) { }
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_MESSAGES_H
