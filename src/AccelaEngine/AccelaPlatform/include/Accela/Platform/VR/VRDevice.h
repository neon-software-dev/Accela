/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_VRDEVICE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_VRDEVICE_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

namespace Accela::Platform
{
    struct ACCELA_PUBLIC VRDevice
    {
        enum class Type
        {
            Headset
        };

        Type type;
        glm::mat4 poseTransform;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_VRDEVICE_H
