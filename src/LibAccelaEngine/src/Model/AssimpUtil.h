/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_MODEL_ASSIMPUTIL_H
#define LIBACCELAENGINE_SRC_MODEL_ASSIMPUTIL_H

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Accela::Engine
{

static inline glm::vec3 ConvertToGLM(const aiVector3D& from)
{
    return {from.x, from.y, from.z};
}

static inline glm::mat4 ConvertToGLM(const aiMatrix4x4& from)
{
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

static inline glm::quat ConvertToGLM(const aiQuaternion& from)
{
    return {from.w, from.x, from.y, from.z};
}

}

#endif //LIBACCELAENGINE_SRC_MODEL_ASSIMPUTIL_H
