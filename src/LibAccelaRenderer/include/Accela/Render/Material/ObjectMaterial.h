/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_OBJECTMATERIAL_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_OBJECTMATERIAL_H

#include "Material.h"

#include <glm/glm.hpp>

#include <memory>

namespace Accela::Render
{
    // Warning: This enum needs to be kept in sync with assimp's aiTextureOp enum
    enum class TextureOp
    {
        Multiply,
        Add,
        Subtract,
        Divide,
        SmoothAdd,
        SignedAdd
    };

    /**
     * Properties associated with an object material
     */
    struct ObjectMaterialProperties
    {
        bool isAffectedByLighting{true};

        glm::vec3 ambientColor{0};
        glm::vec3 diffuseColor{0};
        glm::vec3 specularColor{0};
        float opacity{1.0f};
        float shininess{0.0f};

        TextureId ambientTextureBind{INVALID_ID};
        float ambientTextureBlendFactor{1.0f};
        TextureOp ambientTextureOp{TextureOp::Multiply};

        TextureId diffuseTextureBind{INVALID_ID};
        float diffuseTextureBlendFactor{1.0f};
        TextureOp diffuseTextureOp{TextureOp::Multiply};

        TextureId specularTextureBind{INVALID_ID};
        float specularTextureBlendFactor{1.0f};
        TextureOp specularTextureOp{TextureOp::Multiply};

        TextureId normalTextureBind{INVALID_ID};
    };

    /**
     * A material that can be applied to object renderables
     */
    struct ObjectMaterial : Material
    {
        using Ptr = std::shared_ptr<ObjectMaterial>;

        ObjectMaterial(MaterialId _materialId, const ObjectMaterialProperties& _properties, std::string _tag)
            : Material(Type::Object, _materialId, std::move(_tag))
            , properties(_properties)
        { }

        ObjectMaterialProperties properties;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_OBJECTMATERIAL_H
