/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGE_H

#include <Accela/Common/ImageData.h>

#include <string>
#include <memory>
#include <vector>
#include <expected>

namespace Accela::Platform
{
    /**
     * Base class which provides access to manipulating the data of engine packages
     */
    class Package
    {
        public:

            static constexpr auto VERSION_KEY = "package_version";
            static constexpr auto VERSION = 1;

            using Ptr = std::shared_ptr<Package>;
            using UPtr = std::unique_ptr<Package>;

        public:

            explicit Package(std::string packageName) : m_packageName(std::move(packageName)) {}
            virtual ~Package() = default;

            auto operator<=>(const Package&) const = default;

            [[nodiscard]] std::string GetPackageName() const noexcept { return m_packageName; }

            [[nodiscard]] virtual std::vector<std::string> GetAudioFileNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetFontFileNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetModelFileNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetTextureFileNames() const = 0;
            [[nodiscard]] virtual std::vector<std::string> GetConstructFileNames() const = 0;

            [[nodiscard]] virtual std::expected<std::vector<unsigned char>, unsigned int>
                GetFontData(const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<unsigned char>, unsigned int>
                GetAudioData(const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<unsigned char>, unsigned int>
                GetModelData(const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<Common::ImageData::Ptr, unsigned int>
                GetTextureData(const std::string& fileName) const = 0;
            [[nodiscard]] virtual std::expected<Common::ImageData::Ptr, unsigned int>
                GetModelTextureData(const std::string& modelFileName, const std::string& textureFileName) const = 0;

        protected:

            std::string m_packageName;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_PACKAGE_H
