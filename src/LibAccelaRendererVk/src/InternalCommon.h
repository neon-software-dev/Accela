#ifndef LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H
#define LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    enum class Axis { X, Y, Z };

    // Warning: These are matched to GPU indexing of cube faces, don't reorder them
    enum class CubeFace
    {
        Right, Left, Up, Down, Back, Forward
    };
}

#endif //LIBACCELARENDERERVK_SRC_INTERNALCOMMON_H
