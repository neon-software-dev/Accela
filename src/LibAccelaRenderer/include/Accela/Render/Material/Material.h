#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_MATERIAL_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_MATERIAL_H

#include <Accela/Render/Id.h>

#include <memory>

namespace Accela::Render
{
    /**
     * Base class for materials when can be registered with the renderer
     */
    struct Material
    {
        enum class Type
        {
            Object
        };

        using Ptr = std::shared_ptr<Material>;

        Material(Type _type, MaterialId _materialId, std::string _tag)
            : type(_type)
            , materialId(_materialId)
            , tag(std::move(_tag))
        { }

        virtual ~Material() = default;

        Type type;
        MaterialId materialId{INVALID_ID};
        std::string tag; // Debug tag
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MATERIAL_MATERIAL_H
