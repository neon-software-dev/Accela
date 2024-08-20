/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_COMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_COMPONENT_H

#include <Accela/Common/SharedLib.h>

#include <memory>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC Component
    {
        using Ptr = std::shared_ptr<Component>;

        enum class Type
        {
            Transform,
            ModelRenderable
        };

        virtual ~Component() = default;

        [[nodiscard]] virtual Type GetType() const = 0;
        [[nodiscard]] virtual bool IsComplete() const = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_COMPONENT_H
