/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
#define ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H

#include "Messages.h"

#include "../MessageBasedScene.h"

#include "../SceneMessageListener.h"

#include "../Util/PollingMessageFulfiller.h"

#include <Accela/Engine/Scene/Scene.h>

#include <Accela/Common/Thread/Message.h>

#include <memory>
#include <queue>
#include <mutex>

namespace Accela
{
    /**
     * Scene run by the editor accela instance
     *
     * TODO: EditorScene becomes base interface with EnqueueMessage, subclasses for
     *  different usages (construct view / model view / etc)
     */
    class EditorScene : public MessageBasedScene
    {
        public:

          using Ptr = std::shared_ptr<EditorScene>;

        public:

            //
            // Scene
            //
            [[nodiscard]] std::string GetName() const override { return "EditorScene"; };

            void OnSceneStart(const Engine::IEngineRuntime::Ptr& engine) override;
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;
            void OnMouseWheelEvent(const Platform::MouseWheelEvent& event) override;

        protected:

            void ProcessMessage(const Common::Message::Ptr& message) override;

        private:

            void ProcessSceneQuitCommand(const SceneQuitCommand::Ptr& cmd);
            void ProcessLoadPackageResourcesCommand(const LoadPackageResourcesCommand::Ptr& cmd);
            void ProcessDestroySceneResourcesCommand(const DestroySceneResourcesCommand::Ptr& cmd);
            void ProcessDestroyEntityCommand(const DestroyEntityCommand::Ptr& cmd);
            void ProcessDestroyAllEntitiesCommand(const DestroyAllEntitiesCommand::Ptr& cmd);
            void ProcessCreateEntityCommand(const CreateEntityCommand::Ptr& cmd);
            void ProcessSetEntityComponentCommand(const SetEntityComponentCommand::Ptr& cmd);
            void ProcessRemoveEntityComponentCommand(const RemoveEntityComponentCommand::Ptr& cmd);
            void ProcessRotateCameraCommand(const RotateCameraCommand::Ptr& cmd);
            void ProcessPanCameraCommand(const PanCameraCommand::Ptr& cmd);
            void ProcessScaleCommand(const ScaleCommand::Ptr& cmd);
            void ProcessSetEntityHighlightedCommand(const SetEntityHighlighted::Ptr& cmd);

            void InitCamera();
            void PanCamera(float xPanScalar, float yPanScalar);
            void RotateCamera(float yRotDegrees, float rightRotDegrees);
            void ScaleCamera(float scaleChange);

        private:

            // TODO! Reset on package change
            glm::vec3 m_focusPoint{0,0,0};
            float m_yRot{0.0f};
            float m_rightRot{0.0f};
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
