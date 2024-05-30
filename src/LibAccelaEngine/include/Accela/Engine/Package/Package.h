/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H

#include <memory>
#include <expected>
#include <vector>
#include <cstddef>
#include <string>

namespace Accela::Engine
{
    static constexpr unsigned int PACKAGE_VERSION = 1;

    class Package
    {
        public:

            using Ptr = std::shared_ptr<Package>;
            using UPtr = std::unique_ptr<Package>;

            enum class CreateError
            {
                InvalidPackageFormat,
                UnsupportedVersion,
                ParseFailure
            };

        public:

            [[nodiscard]] static std::expected<UPtr, CreateError> FromBytes(const std::string& packageName,
                                                                            const std::vector<std::byte>& data);

            [[nodiscard]] std::expected<std::vector<std::byte>, bool> ToBytes() const;

        public:

            Package(std::string name, unsigned int packageVersion);

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }
            [[nodiscard]] unsigned int GetPackageVersion() const noexcept { return m_packageVersion; }

        private:

            std::string m_name;
            unsigned int m_packageVersion;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_PACKAGE_H
