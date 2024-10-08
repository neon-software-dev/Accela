/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENTITY_H

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <string>

namespace Accela::Engine
{
    class IEngineRuntime;

    /**
     * Base class for helper Entity classes which handle entity and component
     * creation internally, providing a simpler interface on top
     */
    class ACCELA_PUBLIC Entity
    {
        public:

            using Ptr = std::shared_ptr<Entity>;
            using UPtr = std::unique_ptr<Entity>;
            using CPtr = std::shared_ptr<const Entity>;

        public:

            Entity(std::shared_ptr<IEngineRuntime> engine, std::string sceneName);

            virtual ~Entity() = default;

            virtual void Destroy() = 0;

        protected:

            std::shared_ptr<IEngineRuntime> m_engine;
            std::string m_sceneName;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENTITY_H
