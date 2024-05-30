/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_PACKAGEMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_PACKAGEMODEL_H

#include <nlohmann/json.hpp>

namespace Accela::Engine
{
    /**
     * Represents the contents of a package file (whether on disk or elsewhere)
     */
    struct PackageModel
    {
        unsigned int packageVersion;
    };

    void to_json(nlohmann::json& j, const PackageModel& p)
    {
        j = nlohmann::json{
            {"package_version", p.packageVersion}
        };
    }

    void from_json(const nlohmann::json& j, PackageModel& p)
    {
        j.at("package_version").get_to(p.packageVersion);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_PACKAGEMODEL_H
