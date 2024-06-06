/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SerializeObj.h"

namespace Accela::Engine
{

void to_json(nlohmann::json& j, const ResourceIdentifier& m)
{
    std::string packageName;

    if (m.GetPackageName().has_value())
    {
        packageName = m.GetPackageName()->name;
    }

    j = nlohmann::json{
        {"package_name", packageName},
        {"resource_name", m.GetResourceName()}
    };
}

void from_json(const nlohmann::json& j, ResourceIdentifier& m)
{
    std::string packageName;
    j.at("package_name").get_to(packageName);

    if (!packageName.empty())
    {
        m.SetPackageName(PackageName(packageName));
    }

    std::string resourceName;
    j.at("resource_name").get_to(resourceName);
    m.SetResourceName(resourceName);
}

}

namespace glm
{

void to_json(nlohmann::json& j, const vec3& m)
{
    j = nlohmann::json{
        {"x", m.x},
        {"y", m.y},
        {"z", m.z}
    };
}

void from_json(const nlohmann::json& j, vec3& m)
{
    j.at("x").get_to(m.x);
    j.at("y").get_to(m.y);
    j.at("z").get_to(m.z);
}

void to_json(nlohmann::json& j, const vec4& m)
{
    j = nlohmann::json{
        {"x", m.x},
        {"y", m.y},
        {"z", m.z},
        {"w", m.w},
    };
}

void from_json(const nlohmann::json& j, vec4& m)
{
    j.at("x").get_to(m.x);
    j.at("y").get_to(m.y);
    j.at("z").get_to(m.z);
    j.at("w").get_to(m.w);
}

void to_json(nlohmann::json& j, const quat& m)
{
    j = nlohmann::json{
        {"x", m.x},
        {"y", m.y},
        {"z", m.z},
        {"w", m.w},
    };
}

void from_json(const nlohmann::json& j, quat& m)
{
    j.at("x").get_to(m.x);
    j.at("y").get_to(m.y);
    j.at("z").get_to(m.z);
    j.at("w").get_to(m.w);
}

}