/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/Manifest.h>

#include "ManifestModel.h"

#include "../Util/SerializeObj.h"

#include <nlohmann/json.hpp>

namespace Accela::Engine
{

std::expected<Manifest, Manifest::CreateError> Manifest::FromBytes(const std::vector<std::byte>& data)
{
    ManifestModel model{};

    try
    {
        //
        // Parse the manifest file's contents into a json object
        //
        const nlohmann::json j = nlohmann::json::parse(data.cbegin(), data.cend());

        //
        // Before interpreting the json blob, we at minimum need to look through it for
        // the manifest version field and verify it's a version we support
        //
        if (!j.contains(MANIFEST_VERSION_KEY))
        {
            return std::unexpected(CreateError::InvalidPackageFormat);
        }

        unsigned int manifestVersion{0};
        j.at(MANIFEST_VERSION_KEY).get_to(manifestVersion);

        if (manifestVersion != MANIFEST_VERSION)
        {
            return std::unexpected(CreateError::UnsupportedVersion);
        }

        //
        // This is a supported version package file, interpret it as a PackageModel
        //
        model = j.template get<ManifestModel>();
    }
    catch (nlohmann::json::exception& e)
    {
        return std::unexpected(CreateError::ParseFailure);
    }

    return model.From();
}

Manifest::Manifest(std::string packageName, unsigned int manifestVersion)
    : m_packageName(std::move(packageName))
    , m_manifestVersion(manifestVersion)
{

}

std::expected<std::vector<std::byte>, bool> Manifest::ToBytes() const
{
    return ObjectToBytes<Manifest, ManifestModel>(*this);
}

}
