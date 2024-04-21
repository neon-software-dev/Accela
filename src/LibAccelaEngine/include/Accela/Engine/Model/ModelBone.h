/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELBONE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELBONE_H

#include <string>

#include <glm/glm.hpp>

namespace Accela::Engine
{
    /**
     * Properties of a particular bone within a model
     */
    struct ModelBone
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
