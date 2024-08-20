/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELBONE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELBONE_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <string>

namespace Accela::Engine
{
    /**
     * Properties of a particular bone within a model
     */
    struct ACCELA_PUBLIC ModelBone
    {
        ModelBone(std::string _boneName, const unsigned int& _boneIndex, const glm::mat4& _inverseBindMatrix)
            : boneName(std::move(_boneName))
            , boneIndex(_boneIndex)
            , inverseBindMatrix(_inverseBindMatrix)
        { }

        std::string boneName;           // model name of the bone
        unsigned int boneIndex;         // index of this bone within the model's skeleton
        glm::mat4 inverseBindMatrix;    // bone's bind bose inverse matrix
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELBONE_H
