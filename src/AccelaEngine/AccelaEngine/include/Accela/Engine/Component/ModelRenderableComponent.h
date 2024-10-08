/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_MODELRENDERABLECOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_MODELRENDERABLECOMPONENT_H

#include <Accela/Engine/ResourceIdentifier.h>
#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Common/SharedLib.h>

#include <string>
#include <optional>

namespace Accela::Engine
{
    enum class ModelAnimationType
    {
        Looping,
        OneTime_Reset,
        OneTime_Remain
    };

    /**
     * The current state of a model animation being run
     */
    struct ACCELA_PUBLIC ModelAnimationState
    {
        ModelAnimationState(ModelAnimationType _animationType,
                            std::string _animationName,
                            const double& _animationTime = 0.0)
            : animationType(_animationType)
            , animationName(std::move(_animationName))
            , animationTime(_animationTime)
        { }

        /** Whether the animation is one time or looping */
        ModelAnimationType animationType;

        /** The name of the animation being run */
        std::string animationName;

        /** The current animation timestamp */
        double animationTime;
    };

    /**
     * Allows for attaching a rendered model to an entity
     */
    struct ACCELA_PUBLIC ModelRenderableComponent
    {
        /** The scene the model belongs to */
        std::string sceneName = DEFAULT_SCENE;

        /** The identifier of the model to be displayed */
        ResourceIdentifier modelResource;

        /** Whether the object is included in shadow passes */
        bool shadowPass{true};

        /**
         * Optional animation state to apply to the model. Note: the engine
         * will take care of stepping the animation forwards through time as appropriate.
         */
        std::optional<ModelAnimationState> animationState;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_MODELRENDERABLECOMPONENT_H
