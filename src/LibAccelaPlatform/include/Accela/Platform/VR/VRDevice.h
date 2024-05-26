#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_VRDEVICE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_VR_VRDEVICE_H

#include <glm/glm.hpp>

namespace Accela::Platform
{
    struct VRDevice
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
