/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_CMODELRENDERABLECOMPONENTMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_CMODELRENDERABLECOMPONENTMODEL_H

#include "ComponentModel.h"

#include "../Util/SerializeObj.h"

#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <nlohmann/json.hpp>

#include <memory>

namespace Accela::Engine
{
    struct CModelRenderableComponentModel : public ComponentModel
    {
        using Ptr = std::shared_ptr<CModelRenderableComponentModel>;

        ResourceIdentifier modelResourceIdentifier;

        [[nodiscard]] Component::Type GetType() const override { return Component::Type::ModelRenderable; }

        [[nodiscard]] static CModelRenderableComponentModel::Ptr From(const CModelRenderableComponent::Ptr& component)
        {
            auto model = std::make_shared<CModelRenderableComponentModel>();
            model->modelResourceIdentifier = component->component.modelResource;
            return model;
        }

        [[nodiscard]] Component::Ptr From() const override
        {
            ModelRenderableComponent modelRenderableComponent{};
            modelRenderableComponent.modelResource = modelResourceIdentifier;
            return std::make_shared<CModelRenderableComponent>(modelRenderableComponent);
        }
    };

    void to_json(nlohmann::json& j, const CModelRenderableComponentModel::Ptr& m)
    {
        j = nlohmann::json{
            {"model_identifier", m->modelResourceIdentifier}
        };
    }

    void from_json(const nlohmann::json& j, CModelRenderableComponentModel::Ptr& m)
    {
        j.at("model_identifier").get_to(m->modelResourceIdentifier);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_CMODELRENDERABLECOMPONENTMODEL_H
