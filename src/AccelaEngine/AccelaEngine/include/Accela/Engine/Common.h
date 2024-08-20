/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H

#include <Accela/Common/SharedLib.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>
#include <ostream>

namespace Accela::Engine
{
    using EntityId = std::uint32_t;

    enum class ResultWhen
    {
        /**
         * A resource is ready to be used
         */
        Ready,

        /**
         * A resource is fully loaded into the GPU
         */
        FullyLoaded
    };

    static constexpr auto DEFAULT_NAME_ID = "default";

    struct ACCELA_PUBLIC NameIdType
    {
        NameIdType() = default;
        explicit NameIdType(std::string _name) : name(std::move(_name)) {}
        [[nodiscard]] bool IsDefault() const noexcept { return name == DEFAULT_NAME_ID; }
        auto operator<=>(const NameIdType&) const = default;
        friend std::ostream& operator<<(std::ostream& output, const NameIdType& idc) { output << idc.name; return output; }
        std::string name{DEFAULT_NAME_ID};
    };
}

#define DEFINE_ENGINE_NAME_ID(ID_TYPE) \
namespace Accela::Engine \
{ \
    struct ACCELA_PUBLIC ID_TYPE : public NameIdType{ using NameIdType::NameIdType;  }; \
} \
template<> \
struct std::hash<Accela::Engine::ID_TYPE> \
{ \
    std::size_t operator()(const Accela::Engine::ID_TYPE& o) const noexcept { return std::hash<std::string>{}(o.name); } \
}; \

DEFINE_ENGINE_NAME_ID(PhysicsSceneName)
DEFINE_ENGINE_NAME_ID(PlayerControllerName)
DEFINE_ENGINE_NAME_ID(PackageName)

namespace Accela::Engine
{
    static const PhysicsSceneName DEFAULT_PHYSICS_SCENE = PhysicsSceneName(Engine::DEFAULT_NAME_ID);
    static const PlayerControllerName DEFAULT_PLAYER_NAME = PlayerControllerName(Engine::DEFAULT_NAME_ID);
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMMON_H
