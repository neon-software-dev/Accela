/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H

#include <Accela/Engine/Package/Component.h>

#include <Accela/Engine/Component/TransformComponent.h>

namespace Accela::Engine
{
    struct CTransformComponent : public Component
    {
        using Ptr = std::shared_ptr<CTransformComponent>;

        CTransformComponent() = default;

        explicit CTransformComponent(TransformComponent _component)
            : component(_component)
        { }

        [[nodiscard]] Component::Type GetType() const override { return Component::Type::Transform; }
        [[nodiscard]] bool IsComplete() const override { return true; };

        TransformComponent component;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H
