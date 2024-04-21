/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H

#include <Accela/Render/Texture/TextureSampler.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace Accela::Engine
{
    struct ModelTexture
    {
        ModelTexture(std::string _filename,
                     Render::UVAddressMode _uvAddressMode,
                     const float& _texBlendFactor,
                     const Render::TextureOp& _texOp)
            : fileName(std::move(_filename))
            , uvAddressMode(std::move(_uvAddressMode))
            , texBlendFactor(_texBlendFactor)
            , texOp(_texOp)
        { }

        std::string fileName;
        Render::UVAddressMode uvAddressMode;
        float texBlendFactor;
        Render::TextureOp texOp;
    };

    /**
     * Properties of a specific material that a model uses
     */
    struct ModelMaterial
    {
        std::string name;

        glm::vec3 ambientColor{0};
        glm::vec3 diffuseColor{0};
        glm::vec3 specularColor{0};
        float opacity{1.0f};
        float shininess{0.0f};

        std::vector<ModelTexture> ambientTextures;
        std::vector<ModelTexture> diffuseTextures;
        std::vector<ModelTexture> specularTextures;
        std::vector<ModelTexture> normalTextures;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H
