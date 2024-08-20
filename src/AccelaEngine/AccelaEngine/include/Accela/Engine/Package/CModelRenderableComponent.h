/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CMODELRENDERABLECOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CMODELRENDERABLECOMPONENT_H

#include <Accela/Engine/Package/Component.h>

#include <Accela/Engine/Component/ModelRenderableComponent.h>

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC CModelRenderableComponent : public Component
    {
        using Ptr = std::shared_ptr<CModelRenderableComponent>;

        CModelRenderableComponent() = default;

        explicit CModelRenderableComponent(ModelRenderableComponent _component)
            : component(std::move(_component))
        { }

        [[nodiscard]] Component::Type GetType() const override { return Component::Type::ModelRenderable; }

        [[nodiscard]] bool IsComplete() const override
        {
            return component.modelResource.IsValid();
        };

        ModelRenderableComponent component;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CMODELRENDERABLECOMPONENT_H
