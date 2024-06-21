/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H

#include <cstdint>
#include <ostream>

namespace Accela::Render
{
    using IdType = std::uintmax_t;

    constexpr IdType INVALID_ID = 0;

    struct IdClass
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
namespace Accela::Render \
{ \
    struct ID_TYPE : public IdClass \
    { \
        using IdClass::IdClass; \
        [[nodiscard]] static ID_TYPE Invalid() noexcept { return ID_TYPE{INVALID_ID}; } \
    }; \
} \
template<> \
struct std::hash<Accela::Render::ID_TYPE> \
{ \
    std::size_t operator()(const Accela::Render::ID_TYPE& o) const noexcept { return std::hash<Accela::Render::IdType>{}(o.id); } \
}; \

DEFINE_ID_TYPE(TextureId)
DEFINE_ID_TYPE(FrameBufferId)
DEFINE_ID_TYPE(MeshId)
DEFINE_ID_TYPE(RenderableId)
DEFINE_ID_TYPE(SpriteId)
DEFINE_ID_TYPE(ObjectId)
DEFINE_ID_TYPE(TerrainId)
DEFINE_ID_TYPE(MaterialId)
DEFINE_ID_TYPE(LightId)
DEFINE_ID_TYPE(RenderTargetId)

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_ID_H
