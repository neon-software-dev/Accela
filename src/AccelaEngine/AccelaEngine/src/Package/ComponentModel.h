/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_COMPONENTMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_COMPONENTMODEL_H

#include <Accela/Engine/Package/Component.h>

#include <nlohmann/json.hpp>

#include <memory>

namespace Accela::Engine
{
    struct ComponentModel
    {
        using Ptr = std::shared_ptr<ComponentModel>;

        std::string name;

        virtual ~ComponentModel() = default;

        [[nodiscard]] virtual Component::Type GetType() const = 0;

        [[nodiscard]] static ComponentModel::Ptr From(const Component::Ptr& component);
        [[nodiscard]] virtual Component::Ptr From() const = 0;
    };

    void to_json(nlohmann::json& j, const ComponentModel::Ptr& m);
    void from_json(const nlohmann::json& j, ComponentModel::Ptr& m);
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_COMPONENTMODEL_H
