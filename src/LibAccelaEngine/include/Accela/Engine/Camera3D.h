/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA3D_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA3D_H

#include "Camera.h"

namespace Accela::Engine
{
    /**
     * Camera used for 3D / world space work. Can be manipulated with 3D
     * vectors, has an orientation, and a field of view.
     */
    class Camera3D : public Camera
    {
        public:

            using Ptr = std::shared_ptr<Camera3D>;

        public:

            explicit Camera3D(const glm::vec3& position = {0,0,0}, float fovYDegrees = 45.0f);

            [[nodiscard]] glm::vec3 GetPosition() const override;
            [[nodiscard]] glm::vec3 GetLookUnit() const override;
            [[nodiscard]] glm::vec3 GetUpUnit() const override;
            [[nodiscard]] glm::vec3 GetRightUnit() const override;
            [[nodiscard]] float GetFovYDegrees() const noexcept;

            void TranslateBy(const glm::vec3& translation);
            void SetPosition(const glm::vec3& position) noexcept;
            void RotateBy(float xRotDeg, float yRotDeg);
            void SetFovYDegrees(float fovy) noexcept;

        private:

            float m_fovy{45.0f};
            glm::vec3 m_position{0, 0, 0};
            glm::vec3 m_lookUnit{0, 0, -1};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_CAMERA3D_H
