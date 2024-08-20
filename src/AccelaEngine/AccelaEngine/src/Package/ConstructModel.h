/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_CONSTRUCTMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_CONSTRUCTMODEL_H

#include "CEntityModel.h"

#include <Accela/Engine/Package/Construct.h>

#include <nlohmann/json.hpp>

#include <vector>

namespace Accela::Engine
{
    /**
     * Represents the contents of a construct file (whether on disk or elsewhere)
     */
    struct ConstructModel
    {
        std::string constructName;
        std::vector<CEntityModel> entityModels;

        [[nodiscard]] static ConstructModel From(const Construct& construct)
        {
            ConstructModel model{};
            model.constructName = construct.GetName();

            for (const auto& entity : construct.GetEntities())
            {
                model.entityModels.push_back(CEntityModel::From(entity));
            }

            return model;
        }

        [[nodiscard]] Construct From() const
        {
            Construct construct(constructName);

            for (const auto& entityModel : entityModels)
            {
                construct.AddEntity(entityModel.From());
            }

            return construct;
        }
    };

    void to_json(nlohmann::json& j, const ConstructModel& m)
    {
        j = nlohmann::json{
            {"construct_name", m.constructName},
            {"entities", m.entityModels}
        };
    }

    void from_json(const nlohmann::json& j, ConstructModel& m)
    {
        j.at("construct_name").get_to(m.constructName);
        j.at("entities").get_to(m.entityModels);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_CONSTRUCTMODEL_H
