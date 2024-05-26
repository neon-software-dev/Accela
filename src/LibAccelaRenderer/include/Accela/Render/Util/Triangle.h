#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_TRIANGLE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_TRIANGLE_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    struct Triangle
    {
        Triangle(const glm::vec3& _p1, const glm::vec3& _p2, const glm::vec3& _p3)
            : p1(_p1)
            , p2(_p2)
            , p3(_p3)
        { }

        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_TRIANGLE_H
