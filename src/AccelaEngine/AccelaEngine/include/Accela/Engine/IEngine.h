/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINE_H

#include "Scene/Scene.h"

#include <Accela/Render/RenderInit.h>
#include <Accela/Common/SharedLib.h>

#include <functional>

namespace Accela::Engine
{
    class ACCELA_PUBLIC IEngine
    {
        public:

            virtual ~IEngine() = default;

            /**
             * Give control over the thread of execution to the Accela engine, running
             * the provided scene by default, until all scenes finish, at which point
             * the function will return.
             *
             * @param initialScene The initial scene to be run
             * @param renderOutputMode Whether the renderer should output to a connected VR headset
             *
             * TODO: supportVRHeadset -> more complete RunSettings or such
             */
            virtual void Run(Scene::UPtr initialScene,
                             Render::OutputMode renderOutputMode,
                             const std::function<void()>& onInitCallback) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IENGINE_H
