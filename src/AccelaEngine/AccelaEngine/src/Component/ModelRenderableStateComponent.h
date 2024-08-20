/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_MODELRENDERABLESTATECOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_MODELRENDERABLESTATECOMPONENT_H

#include <Accela/Engine/ResourceIdentifier.h>

#include "../Model/ModelPose.h"

#include <optional>

namespace Accela::Engine
{
    struct ModelRenderableStateComponent
    {
        explicit ModelRenderableStateComponent(ResourceIdentifier _modelResource)
            : modelResource(std::move(_modelResource))
        { }

        ResourceIdentifier modelResource;
        std::optional<ModelPose> modelPose;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_MODELRENDERABLESTATECOMPONENT_H
