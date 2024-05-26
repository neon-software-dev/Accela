#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_RECT_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_RECT_H

#include <glm/glm.hpp>

#include <memory>
#include <type_traits>
#include <cstdint>
#include <utility>

namespace Accela::Render
{
    /**
     * Helper class which defines a size (width/height)
     *
     * @tparam S Data type to use for the dimensions
     */
    template <typename S>
    struct Size
    {
        Size() = default;

        Size(const S& _w, const S& _h)
                : w(_w)
                , h(_h)
        { }

        explicit Size(const std::pair<S, S>& s)
                : w(s.first)
                , h(s.second)
        { }

        bool operator==(const Size<S>& rhs)
        {
            return
                w == rhs.w &&
                h == rhs.h;
        }

        S w{}; S h{};
    };

    /**
     * Helper class which defines a Rect located in 2D space.
     *
     * @tparam P Data type to use for the Rect's position
     * @tparam S Data type to use for the Rect's size
     */
    template <typename P, typename S>
    struct Rect
    {
        Rect() = default;

        Rect(const S& _w, const S& _h)
            : x(0)
            , y(0)
            , w(_w)
            , h(_h)
        { }

        explicit Rect(const std::pair<S, S>& p)
                : x(0)
                , y(0)
                , w(p.first)
                , h(p.second)
        { }

        explicit Rect(const Size<S>& s)
                : x(0)
                , y(0)
                , w(s.w)
                , h(s.h)
        { }

        Rect(const P& _x, const P& _y, const S& _w, const S& _h)
            : x(_x)
            , y(_y)
            , w(_w)
            , h(_h)
        { }

        bool operator==(const Rect<P,S>& rhs)
        {
            return
                x == rhs.x &&
                y == rhs.y &&
                w == rhs.w &&
                h == rhs.h;
        }

        Size<S> GetSize() const noexcept
        {
            return Size<S>(w, h);
        }

        P x{}; P y{};
        S w{}; S h{};
    };

    using USize         = Size<uint32_t>;
    using FSize         = Size<float>;

    using URect         = Rect<uint32_t, uint32_t>;
    using IRect         = Rect<int32_t, int32_t>;
    using FRect         = Rect<float, float>;
    using Viewport      = URect;
    using ScreenRect    = IRect;
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_RECT_H
