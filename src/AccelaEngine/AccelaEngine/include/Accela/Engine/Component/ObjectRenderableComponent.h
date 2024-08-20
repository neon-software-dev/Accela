/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_OBJECTRENDERABLECOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_OBJECTRENDERABLECOMPONENT_H

#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Render/Id.h>

#include <Accela/Common/SharedLib.h>

#include <string>

namespace Accela::Engine
{
    /**
     * Allows for attaching a custom mesh/material to an entity
     */
    struct ACCELA_PUBLIC ObjectRenderableComponent
    {
        /** The scene the object belongs to */
        std::string sceneName = DEFAULT_SCENE;

        /** The id of the mesh to be displayed */
        Render::MeshId meshId{Render::INVALID_ID};

        /** The id of the material to be applied to the mesh */
        Render::MaterialId materialId{Render::INVALID_ID};

        /** Whether the object is included in shadow passes */
        bool shadowPass{true};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_OBJECTRENDERABLECOMPONENT_H
