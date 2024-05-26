/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/SceneEntity.h>

#include "WrappedSceneCallbacks.h"

namespace Accela::Engine
{

SceneEntity::SceneEntity(std::shared_ptr<IEngineRuntime> engine, std::string sceneName, SceneEvents::Ptr sceneEvents)
    : Entity(std::move(engine), std::move(sceneName))
    , m_sceneEvents(std::move(sceneEvents))
{
    m_wrappedSceneCalls = std::make_shared<WrappedSceneCallbacks>(this);
    m_sceneEvents->RegisterListener(m_wrappedSceneCalls);
}

SceneEntity::~SceneEntity()
{
    m_sceneEvents->DeregisterListener(m_wrappedSceneCalls);
    m_sceneEvents = nullptr;

    m_wrappedSceneCalls = nullptr;
}

}
