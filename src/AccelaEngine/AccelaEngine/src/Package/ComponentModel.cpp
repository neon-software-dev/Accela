/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ComponentModel.h"

#include "CTransformComponentModel.h"
#include "CModelRenderableComponentModel.h"

#include <cassert>

namespace Accela::Engine
{

ComponentModel::Ptr ComponentModel::From(const Component::Ptr& component)
{
    switch (component->GetType())
    {
        case Component::Type::Transform:
            return CTransformComponentModel::From(std::dynamic_pointer_cast<CTransformComponent>(component));
        case Component::Type::ModelRenderable:
            return CModelRenderableComponentModel::From(std::dynamic_pointer_cast<CModelRenderableComponent>(component));
    }

    assert(false);
    return nullptr;
}

void to_json(nlohmann::json& j, const ComponentModel::Ptr& m)
{
    switch (m->GetType())
    {
        case Component::Type::Transform:
            j = nlohmann::json{
                {"type", "transform"},
                {"data", std::dynamic_pointer_cast<CTransformComponentModel>(m)}
            };
        break;

        case Component::Type::ModelRenderable:
            j = nlohmann::json{
                {"type", "model_renderable"},
                {"data", std::dynamic_pointer_cast<CModelRenderableComponentModel>(m)}
            };
        break;
    }
}

void from_json(const nlohmann::json& j, ComponentModel::Ptr& m)
{
    std::string type;
    j.at("type").get_to(type);

    if (type == "transform")
    {
        m = std::make_shared<CTransformComponentModel>();
        auto typedM = std::dynamic_pointer_cast<CTransformComponentModel>(m);
        j.at("data").get_to(typedM);
    }
    else if (type == "model_renderable")
    {
        m = std::make_shared<CModelRenderableComponentModel>();
        auto typedM = std::dynamic_pointer_cast<CModelRenderableComponentModel>(m);
        j.at("data").get_to(typedM);
    }
    else
    {
        assert(false);
    }
}

}
