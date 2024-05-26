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
