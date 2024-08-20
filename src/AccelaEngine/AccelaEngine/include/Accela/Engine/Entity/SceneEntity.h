/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCENEENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCENEENTITY_H

#include <Accela/Engine/Entity/Entity.h>
#include <Accela/Engine/Scene/SceneCallbacks.h>
#include <Accela/Engine/Scene/SceneEvents.h>

#include <Accela/Common/SharedLib.h>

#include <memory>

namespace Accela::Engine
{
    /**
     * An Entity which registers itself with a scene to receive scene event callbacks
     */
    class ACCELA_PUBLIC SceneEntity : public Entity, public SceneCallbacks
    {
        public:

            explicit SceneEntity(std::shared_ptr<IEngineRuntime> engine,
                                 std::string sceneName,
                                 SceneEvents::Ptr sceneEvents);

            ~SceneEntity() override;

        private:

            SceneEvents::Ptr m_sceneEvents;

            std::shared_ptr<SceneCallbacks> m_wrappedSceneCalls;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_SCENEENTITY_H
