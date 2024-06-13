/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H

#include <Accela/Render/Texture/TextureSampler.h>
#include <Accela/Render/Material/ObjectMaterial.h>

#include <Accela/Common/ImageData.h>

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <optional>
#include <cstddef>

namespace Accela::Engine
{
    struct ModelEmbeddedData
    {
        std::vector<std::byte> data;
        std::size_t dataWidth{0};
        std::size_t dataHeight{0};
        std::optional<std::string> dataFormat;
    };

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

        std::optional<ModelEmbeddedData> embeddedData;
    };

    /**
     * Properties of a specific material that a model uses
     */
    struct ModelMaterial
    {
        std::string name;

        glm::vec4 ambientColor{0};
        glm::vec4 diffuseColor{0};
        glm::vec4 specularColor{0};
        float opacity{1.0f};
        std::optional<Render::AlphaMode> alphaMode;
        std::optional<float> alphaCutoff;
        float shininess{0.0f};
        bool twoSided{false};

        std::vector<ModelTexture> ambientTextures;
        std::vector<ModelTexture> diffuseTextures;
        std::vector<ModelTexture> specularTextures;
        std::vector<ModelTexture> normalTextures;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELMATERIAL_H
