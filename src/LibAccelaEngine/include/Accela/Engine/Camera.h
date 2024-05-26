/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA_H

#include <glm/glm.hpp>

#include <memory>

namespace Accela::Engine
{
    /**
     * Abstract base class defining a camera
     */
    class Camera
    {
        public:

            using Ptr = std::shared_ptr<Camera>;

        public:

            virtual ~Camera() = default;

            [[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
            [[nodiscard]] virtual glm::vec3 GetLookUnit() const = 0;
            [[nodiscard]] virtual glm::vec3 GetUpUnit() const = 0;
            [[nodiscard]] virtual glm::vec3 GetRightUnit() const = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA_H
