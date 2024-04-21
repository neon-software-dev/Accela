/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MATERIAL_OBJECTMATERIALPAYLOAD_H
#define LIBACCELARENDERERVK_SRC_MATERIAL_OBJECTMATERIALPAYLOAD_H

#include <Accela/Render/Material/ObjectMaterial.h>

#include <glm/glm.hpp>

#include <cstdint>

namespace Accela::Render
{
    struct ObjectMaterialPayload
    {
        alignas(4) uint32_t isAffectedByLighting{0};

        alignas(16) glm::vec3 ambientColor{0};
        alignas(16) glm::vec3 diffuseColor{0};
        alignas(16) glm::vec3 specularColor{0};
        alignas(4) float opacity{1.0f};
        alignas(4) float shininess{0.0f};

        alignas(4) uint32_t hasAmbientTexture{0};
        alignas(4) float ambientTextureBlendFactor{0.0f};
        alignas(4) uint32_t ambientTextureOp{0};

        alignas(4) uint32_t hasDiffuseTexture{0};
        alignas(4) float diffuseTextureBlendFactor{0.0f};
        alignas(4) uint32_t diffuseTextureOp{0};

        alignas(4) uint32_t hasSpecularTexture{0};
        alignas(4) float specularTextureBlendFactor{0.0f};
        alignas(4) uint32_t specularTextureOp{0};

        alignas(4) uint32_t hasNormalTexture{0};
    };
}

#endif //LIBACCELARENDERERVK_SRC_MATERIAL_OBJECTMATERIALPAYLOAD_H
