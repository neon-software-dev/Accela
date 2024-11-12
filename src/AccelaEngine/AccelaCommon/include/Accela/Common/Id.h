/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELACOMMON_INCLUDE_ACCELA_COMMON_ID_H
#define ACCELAENGINE_ACCELACOMMON_INCLUDE_ACCELA_COMMON_ID_H

#include "SharedLib.h"

#include <cstdint>
#include <ostream>

namespace Accela
{
    using IdType = std::uint32_t;

    constexpr IdType INVALID_ID = 0;
}

namespace Accela::Common
{
    struct ACCELA_PUBLIC IdClass
    {
        IdClass() = default;

        explicit IdClass(IdType _id) : id(_id) {}

        [[nodiscard]] bool IsValid() const noexcept { return id != INVALID_ID; }

        auto operator<=>(const IdClass&) const = default;
        IdClass& operator++() { id++; return *this; }
        IdClass operator++(int) { IdClass temp = *this; ++*this; return temp; }
        friend std::ostream& operator<<(std::ostream& output, const IdClass& idc) { output << idc.id; return output; }

        IdType id{INVALID_ID};
    };
}

#define DEFINE_ID_TYPE(ID_TYPE) \
struct ACCELA_PUBLIC ID_TYPE : public Accela::Common::IdClass \
{ \
    using Accela::Common::IdClass::IdClass; \
    [[nodiscard]] static ID_TYPE Invalid() noexcept { return ID_TYPE{Accela::INVALID_ID}; } \
}; \

#define DEFINE_ID_HASH(ID_TYPE) \
template<> \
struct std::hash<ID_TYPE> \
{ \
    std::size_t operator()(const ID_TYPE& o) const noexcept { return std::hash<Accela::IdType>{}(o.id); } \
}; \

#endif //ACCELAENGINE_ACCELACOMMON_INCLUDE_ACCELA_COMMON_ID_H
