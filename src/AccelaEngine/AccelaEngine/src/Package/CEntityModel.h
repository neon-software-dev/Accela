/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_CENTITYMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_CENTITYMODEL_H

#include <Accela/Engine/Package/CEntity.h>

#include "ComponentModel.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Accela::Engine
{
    struct CEntityModel
    {
        std::string name;
        std::vector<ComponentModel::Ptr> components;

        [[nodiscard]] static CEntityModel From(const CEntity::Ptr& entity)
        {
            CEntityModel model{};
            model.name = entity->name;

            for (const auto& component : entity->components)
            {
                model.components.push_back(ComponentModel::From(component));
            }

            return model;
        }

        [[nodiscard]] CEntity::Ptr From() const
        {
            std::vector<Component::Ptr> realComponents;

            for (const auto& modelComponent : components)
            {
                realComponents.push_back(modelComponent->From());
            }

            return std::make_shared<CEntity>(name, realComponents);
        }
    };

    void to_json(nlohmann::json& j, const CEntityModel& m)
    {
        j = nlohmann::json{
            {"entity_name", m.name},
            {"components", m.components}
        };
    }

    void from_json(const nlohmann::json& j, CEntityModel& m)
    {
        j.at("entity_name").get_to(m.name);
        j.at("components").get_to(m.components);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_CENTITYMODEL_H
