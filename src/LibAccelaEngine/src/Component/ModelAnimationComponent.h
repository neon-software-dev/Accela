/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_MODELANIMATIONCOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_MODELANIMATIONCOMPONENT_H

#include "../Model/ModelPose.h"

#include <optional>

namespace Accela::Engine
{
    struct ModelAnimationComponent
    {
        std::optional<ModelPose> modelPose;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_MODELANIMATIONCOMPONENT_H
