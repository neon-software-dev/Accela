/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/Entity.h>

namespace Accela::Engine
{

Entity::Entity(std::shared_ptr<IEngineRuntime> engine, std::string sceneName)
    : m_engine(std::move(engine))
    , m_sceneName(std::move(sceneName))
{

}

}
