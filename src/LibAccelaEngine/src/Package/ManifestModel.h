/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PACKAGE_MANIFESTMODEL_H
#define LIBACCELAENGINE_SRC_PACKAGE_MANIFESTMODEL_H

#include <Accela/Engine/Package/Manifest.h>

#include <nlohmann/json.hpp>

namespace Accela::Engine
{
    static constexpr auto MANIFEST_VERSION_KEY = "manifest_version";

    /**
     * Represents the contents of a package manifest file (whether on disk or elsewhere)
     */
    struct ManifestModel
    {
        unsigned int manifestVersion;
        std::string packageName;

        [[nodiscard]] static ManifestModel From(const Manifest& manifest)
        {
            ManifestModel model{};
            model.manifestVersion = manifest.GetManifestVersion();
            model.packageName = manifest.GetPackageName();
            return model;
        }

        [[nodiscard]] Manifest From() const
        {
            return {packageName, manifestVersion};
        }
    };

    void to_json(nlohmann::json& j, const ManifestModel& m)
    {
        j = nlohmann::json{
            {MANIFEST_VERSION_KEY, m.manifestVersion},
            {"package_name", m.packageName}
        };
    }

    void from_json(const nlohmann::json& j, ManifestModel& m)
    {
        j.at(MANIFEST_VERSION_KEY).get_to(m.manifestVersion);
        j.at("package_name").get_to(m.packageName);
    }
}

#endif //LIBACCELAENGINE_SRC_PACKAGE_MANIFESTMODEL_H
