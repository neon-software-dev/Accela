/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA2D_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA2D_H

#include "Camera.h"

#include <optional>

namespace Accela::Engine
{
    /**
     * Camera use for 2D / screen space camera work. Can be manipulated with 2D vectors
     * and can have bounds set on which areas of the screen the camera is allowed to
     * move over.
     */
    class Camera2D : public Camera
    {
        public:

            using Ptr = std::shared_ptr<Camera2D>;

        public:

            [[nodiscard]] glm::vec3 GetPosition() const override;
            [[nodiscard]] glm::vec3 GetLookUnit() const override;
            [[nodiscard]] glm::vec3 GetUpUnit() const override;
            [[nodiscard]] glm::vec3 GetRightUnit() const override;

            void TranslateBy(const glm::vec2& translation) noexcept;
            void SetPosition(const glm::vec2& position) noexcept;

            /**
             * Constrain the camera to the specified bounds
             *
             * @param topLeft Top-left of the allowed area
             * @param bottomRight Bottom-right of the allowed area
             */
            void SetBounds(const glm::vec2& topLeft, const glm::vec2& bottomRight);

        private:

            void EnforceBounds();

        private:

            glm::vec3 m_position{0, 0, 0};

            std::optional<glm::vec2> m_topLeftBound;
            std::optional<glm::vec2> m_bottomRightBound;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA2D_H
