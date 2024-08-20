/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_LIGHTCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_LIGHTCOMPONENT_H

#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Render/Light.h>

#include <Accela/Common/SharedLib.h>

#include <string>

namespace Accela::Engine
{
    /**
     * Allows for attaching a light effect to an entity
     */
    struct ACCELA_PUBLIC LightComponent
    {
        explicit LightComponent(Render::LightProperties _lightProperties)
            : lightProperties(_lightProperties)
        { }

        /** The scene the light belongs to */
        std::string sceneName = DEFAULT_SCENE;

        /** Whether the light casts shadows */
        bool castsShadows{true};

        /** The light's properties */
        Render::LightProperties lightProperties;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_LIGHTCOMPONENT_H
