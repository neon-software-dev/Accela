#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H

#include <Accela/Engine/ResourceIdentifier.h>

namespace Accela::Engine
{
    /**
     * Static-Mesh-sourced bounds for a physics object
     */
    struct Bounds_StaticMesh
    {
        /**
         * @param _resource Identifies the mesh resource to use as bounds
         */
        explicit Bounds_StaticMesh(ResourceIdentifier _resource)
            : resource(std::move(_resource))
        { }

        ResourceIdentifier resource;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_BOUNDS_BOUNDS_STATICMESH_H
