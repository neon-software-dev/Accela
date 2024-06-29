/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H

#include <Accela/Engine/Package/Component.h>

#include <Accela/Engine/Component/TransformComponent.h>

#include <glm/glm.hpp>

namespace Accela::Engine
{
    struct CTransformComponent : public Component
    {
        using Ptr = std::shared_ptr<CTransformComponent>;

        CTransformComponent() = default;
        CTransformComponent(const glm::vec3& _position, const glm::vec3& _eulerRotation, const glm::vec3& _scale)
            : position(_position)
            , eulerRotation(_eulerRotation)
            , scale(_scale)
        { }

        [[nodiscard]] Component::Type GetType() const override { return Component::Type::Transform; }
        [[nodiscard]] bool IsComplete() const override { return true; };

        [[nodiscard]] TransformComponent ToEngineComponent() const
        {
            Engine::TransformComponent transformComponent{};
            transformComponent.SetPosition(position);

            transformComponent.SetOrientation(
                glm::angleAxis(glm::radians(eulerRotation.x), glm::vec3(1,0,0)) *
                glm::angleAxis(glm::radians(eulerRotation.y), glm::vec3(0,1,0)) *
                glm::angleAxis(glm::radians(eulerRotation.z), glm::vec3(0,0,1))
            );

            // Divided by 100 to convert from editor 100% to engine 1.0f
            transformComponent.SetScale(scale / 100.0f);

            return transformComponent;
        }

        glm::vec3 position{0.0f};
        // Stored as euler angles for use in editor, converted to quaternions at engine interface
        glm::vec3 eulerRotation{0.0f};
        glm::vec3 scale{100.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PACKAGE_CTRANSFORMCOMPONENT_H
