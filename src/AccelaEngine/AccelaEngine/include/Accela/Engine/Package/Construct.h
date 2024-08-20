/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H

#include "CEntity.h"

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <expected>
#include <vector>
#include <cstddef>
#include <string>

namespace Accela::Engine
{
    class ACCELA_PUBLIC Construct
    {
        public:

            using Ptr = std::shared_ptr<Construct>;
            using UPtr = std::unique_ptr<Construct>;

        public:

            [[nodiscard]] static std::expected<Ptr, bool> FromBytes(const std::vector<std::byte>& data);

            [[nodiscard]] std::expected<std::vector<std::byte>, bool> ToBytes() const;

        public:

            explicit Construct(std::string name);

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }
            [[nodiscard]] const std::vector<CEntity::Ptr>& GetEntities() const noexcept { return m_entities; }

            void AddEntity(const CEntity::Ptr& entity);
            void RemoveEntity(const std::string& entityName);

        private:

            std::string m_name;

            std::vector<CEntity::Ptr> m_entities;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CONSTRUCT_H
