/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Render/Material/ObjectMaterial.h>

#include <optional>

namespace Accela::Engine
{
    struct ObjectMaterialProperties
    {
        bool isAffectedByLighting{true};

        glm::vec3 ambientColor{0};
        glm::vec3 diffuseColor{0};
        glm::vec3 specularColor{0};
        float opacity{1.0f};
        Render::AlphaMode alphaMode{Render::AlphaMode::Opaque};
        float alphaCutoff{1.0f};
        float shininess{0.0f};

        std::optional<ResourceIdentifier> ambientTexture;
        float ambientTextureBlendFactor{1.0f};
        Render::TextureOp ambientTextureOp{Render::TextureOp::Multiply};

        std::optional<ResourceIdentifier> diffuseTexture;
        float diffuseTextureBlendFactor{1.0f};
        Render::TextureOp diffuseTextureOp{Render::TextureOp::Multiply};

        std::optional<ResourceIdentifier> specularTexture;
        float specularTextureBlendFactor{1.0f};
        Render::TextureOp specularTextureOp{Render::TextureOp::Multiply};

        std::optional<ResourceIdentifier> normalTexture;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H
