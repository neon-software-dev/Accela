/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_CTRANSFORMCOMPONENTMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_CTRANSFORMCOMPONENTMODEL_H

#include "ComponentModel.h"

#include "../Util/SerializeObj.h"

#include <Accela/Engine/Package/CTransformComponent.h>

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>

#include <memory>

namespace Accela::Engine
{
    struct CTransformComponentModel : public ComponentModel
    {
        using Ptr = std::shared_ptr<CTransformComponentModel>;

        glm::vec3 position{0.0f};
        glm::quat orientation{glm::identity<glm::quat>()};
        glm::vec3 scale{1.0f};

        [[nodiscard]] Component::Type GetType() const override { return Component::Type::Transform; }

        [[nodiscard]] static CTransformComponentModel::Ptr From(const CTransformComponent::Ptr& component)
        {
            auto model = std::make_shared<CTransformComponentModel>();
            model->position = component->component.GetPosition();
            model->orientation = component->component.GetOrientation();
            model->scale = component->component.GetScale();
            return model;
        }

        [[nodiscard]] Component::Ptr From() const override
        {
            TransformComponent transformComponent{};
            transformComponent.SetPosition(position);
            transformComponent.SetOrientation(orientation);
            transformComponent.SetScale(scale);
            return std::make_shared<CTransformComponent>(transformComponent);
        }
    };

    void to_json(nlohmann::json& j, const CTransformComponentModel::Ptr& m)
    {
        j = nlohmann::json{
            {"position", m->position},
            {"orientation", m->orientation},
            {"scale", m->scale}
        };
    }

    void from_json(const nlohmann::json& j, CTransformComponentModel::Ptr& m)
    {
        j.at("position").get_to(m->position);
        j.at("orientation").get_to(m->orientation);
        j.at("scale").get_to(m->scale);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_CTRANSFORMCOMPONENTMODEL_H
