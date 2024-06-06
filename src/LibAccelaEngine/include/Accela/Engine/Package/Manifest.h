/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_MANIFEST_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_MANIFEST_H

#include <memory>
#include <expected>
#include <vector>
#include <cstddef>
#include <string>

namespace Accela::Engine
{
    static constexpr unsigned int MANIFEST_VERSION = 1;

    class Manifest
    {
        public:

            enum class CreateError
            {
                InvalidPackageFormat,
                UnsupportedVersion,
                ParseFailure
            };

        public:

            [[nodiscard]] static std::expected<Manifest, CreateError> FromBytes(const std::vector<std::byte>& data);

            [[nodiscard]] std::expected<std::vector<std::byte>, bool> ToBytes() const;

        public:

            Manifest() = default;

            Manifest(std::string packageName, unsigned int manifestVersion);

            auto operator<=>(const Manifest&) const = default;

            [[nodiscard]] std::string GetPackageName() const noexcept { return m_packageName; }
            [[nodiscard]] unsigned int GetManifestVersion() const noexcept { return m_manifestVersion; }

        private:

            std::string m_packageName;
            unsigned int m_manifestVersion{MANIFEST_VERSION};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_MANIFEST_H
