/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H

#include <memory>
#include <expected>
#include <vector>
#include <cstddef>
#include <string>

namespace Accela::Engine
{
    class Construct
    {
        public:

            using Ptr = std::shared_ptr<Construct>;
            using UPtr = std::unique_ptr<Construct>;

        public:

            [[nodiscard]] static std::expected<Ptr, bool> FromBytes(const std::string& constructName,
                                                                     const std::vector<std::byte>& data);

            [[nodiscard]] std::expected<std::vector<std::byte>, bool> ToBytes() const;

        public:

            explicit Construct(std::string name);

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }

        private:

            std::string m_name;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H
