#ifndef LIBACCELARENDERERVK_SRC_RENDERABLES_RENDERABLEDATA_H
#define LIBACCELARENDERERVK_SRC_RENDERABLES_RENDERABLEDATA_H

#include "../Util/AABB.h"

namespace Accela::Render
{
    template <class T>
    struct RenderableData
    {
        bool isValid{true};
        T renderable{};
        AABB boundingBox_worldSpace{};
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERABLES_RENDERABLEDATA_H
