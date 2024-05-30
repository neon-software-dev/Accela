/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Package/Package.h>

#include "PackageModel.h"

#include <nlohmann/json.hpp>

namespace Accela::Engine
{

std::expected<Package::UPtr, Package::CreateError> Package::FromBytes(const std::string& packageName,
                                                                      const std::vector<std::byte>& data)
{
    PackageModel model{};

    try
    {
        //
        // Parse the package file contents into a json object
        //
        const nlohmann::json j = nlohmann::json::parse(data.cbegin(), data.cend());

        //
        // Before interpreting the json blob, we at minimum need to look through it for
        // the package version field and verify it's a version we support
        //
        if (!j.contains("package_version"))
        {
            return std::unexpected(CreateError::InvalidPackageFormat);
        }

        unsigned int packageVersion{0};
        j.at("package_version").get_to(packageVersion);

        if (packageVersion != 1)
        {
            return std::unexpected(CreateError::UnsupportedVersion);
        }

        //
        // This is a supported version package file, interpret it as a PackageModel
        //
        model = j.template get<PackageModel>();
    }
    catch (nlohmann::json::exception& e)
    {
        return std::unexpected(CreateError::ParseFailure);
    }

    return std::make_unique<Package>(packageName, model.packageVersion);
}

Package::Package(std::string name, unsigned int packageVersion)
    : m_name(std::move(name))
    , m_packageVersion(packageVersion)
{

}

std::expected<std::vector<std::byte>, bool> Package::ToBytes() const
{
    PackageModel model{
        .packageVersion = m_packageVersion
    };

    std::string jsonStr;

    try
    {
        const nlohmann::json j = model;
        jsonStr = to_string(j);
    }
    catch (nlohmann::json::exception& e)
    {
        return std::unexpected(false);
    }

    std::vector<std::byte> byteBuffer(jsonStr.length());
    memcpy(byteBuffer.data(), jsonStr.data(), jsonStr.length());

    return byteBuffer;
}

}
