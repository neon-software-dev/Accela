/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGESOURCE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGESOURCE_H

#include <Accela/Common/ImageData.h>

#include <string>
#include <memory>
#include <vector>
#include <expected>
#include <cstddef>

namespace Accela::Platform
{
    /**
     * Base class which provides access to manipulating the data of engine packages
     */
    class PackageSource
    {
        public:

            static constexpr auto VERSION_KEY = "package_version";
            static constexpr auto VERSION = 1;

            using Ptr = std::shared_ptr<PackageSource>;
            using UPtr = std::unique_ptr<PackageSource>;

        public:

            explicit PackageSource(std::string packageName) : m_packageName(std::move(packageName)) {}
            virtual ~PackageSource() = default;

            auto operator<=>(const PackageSource&) const = default;

            [[nodiscard]] std::string GetPackageName() const noexcept { return m_packageName; }

            [[nodiscard]] virtual std::vector<std::string> GetAudioResourceNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetFontResourceNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetModelResourceNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetTextureResourceNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetConstructResourceNames() const = 0;

            [[nodiscard]] virtual std::expected<std::string, bool> GetTextureFormatHint(const std::string& resourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::string, bool> GetModelTextureFormatHint(const std::string& modelResourceName,
                                                                                             const std::string& resourceName) const = 0;

            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetManifestFileData() const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetFontData(const std::string& resourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetAudioData(const std::string& resourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetModelData(const std::string& resourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetTextureData(const std::string& resourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetModelTextureData(const std::string& modelResourceName,
                                                                                                          const std::string& textureResourceName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, unsigned int> GetConstructData(const std::string& constructName) const = 0;

        protected:

            std::string m_packageName;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGESOURCE_H
