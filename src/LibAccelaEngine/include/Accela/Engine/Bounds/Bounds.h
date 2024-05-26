#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_H

#include <Accela/Engine/Bounds/Bounds_AABB.h>
#include <Accela/Engine/Bounds/Bounds_Sphere.h>
#include <Accela/Engine/Bounds/Bounds_Capsule.h>
#include <Accela/Engine/Bounds/Bounds_StaticMesh.h>
#include <Accela/Engine/Bounds/Bounds_HeightMap.h>

#include <variant>

namespace Accela::Engine
{
    using BoundsVariant = std::variant<
        Bounds_AABB,
        Bounds_Sphere,
        Bounds_Capsule,
        Bounds_StaticMesh,
        Bounds_HeightMap
    >;
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_H
