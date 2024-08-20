/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CENTITY_H

#include "Component.h"

#include <Accela/Common/SharedLib.h>

#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace Accela::Engine
{
    static const std::string DEFAULT_CENTITY_NAME = "New Entity";

    /**
     * A construct entity (as opposed to an 'Entity' which is an engine
     * base class for a helper that wraps ECS operations)
     */
    struct ACCELA_PUBLIC CEntity
    {
        using Ptr = std::shared_ptr<CEntity>;

        explicit CEntity(std::string _name, std::vector<Component::Ptr> _components = {})
            : name(std::move(_name))
            , components(std::move(_components))
        { }

        [[nodiscard]] std::optional<Component::Ptr> GetComponent(const Component::Type& type) const
        {
            const auto it = std::ranges::find_if(components, [&](const auto& component){
                return component->GetType() == type;
            });
            return it != components.cend() ? (std::optional<Component::Ptr>)*it : std::nullopt;
        }

        std::string name;
        std::vector<Component::Ptr> components;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CENTITY_H
