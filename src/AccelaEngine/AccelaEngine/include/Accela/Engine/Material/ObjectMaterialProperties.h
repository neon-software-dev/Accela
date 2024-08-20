/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Render/Material/ObjectMaterial.h>

#include <Accela/Common/SharedLib.h>

#include <optional>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC ObjectMaterialProperties
    {
        bool isAffectedByLighting{true};

        glm::vec4 ambientColor{0};
        glm::vec4 diffuseColor{0};
        glm::vec4 specularColor{0};
        float opacity{1.0f};
        Render::AlphaMode alphaMode{Render::AlphaMode::Opaque};
        float alphaCutoff{1.0f};
        float shininess{0.0f};
        bool twoSided{false};

        std::optional<Render::TextureId> ambientTexture;
        float ambientTextureBlendFactor{1.0f};
        Render::TextureOp ambientTextureOp{Render::TextureOp::Multiply};

        std::optional<Render::TextureId> diffuseTexture;
        float diffuseTextureBlendFactor{1.0f};
        Render::TextureOp diffuseTextureOp{Render::TextureOp::Multiply};

        std::optional<Render::TextureId> specularTexture;
        float specularTextureBlendFactor{1.0f};
        Render::TextureOp specularTextureOp{Render::TextureOp::Multiply};

        std::optional<Render::TextureId> normalTexture;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MATERIAL_OBJECTMATERIALPROPERTIES_H
