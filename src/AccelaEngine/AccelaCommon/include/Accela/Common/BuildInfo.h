/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILDINFO_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILDINFO_H

#include <Accela/Common/SharedLib.h>

namespace Accela::Common
{
    enum class OS
    {
        Windows,
        Linux,
        Unknown
    };

    enum class Platform
    {
        Desktop,
        Android,
        Unspecified
    };

    /**
     * Helper struct which provides basic information about the current build
     */
    struct ACCELA_PUBLIC BuildInfo
    {
        /**
         * @return Whether or not the current build is a Debug build
         */
        static bool IsDebugBuild()
        {
            #if defined(NDEBUG)
                    return false;
            #else
                    return true;
            #endif
        }

        /**
         * @return The OS the current device is running
         */
        static OS GetOS()
        {
            #if defined(_WIN32) || defined(_WIN64)
                    return OS::Windows;
            #elif defined(__linux__) || defined(__unix__)
                    return OS::Linux;
            #else
                    return OS::Unknown;
            #endif
        }

        /**
         * @return The platform the program was built for
         */
        static Platform GetPlatform()
        {
            #ifdef _ACCELA_PLATFORM_DESKTOP
                    return Platform::Desktop;
            #elif _ACCELA_PLATFORM_ANDROID
                    return Platform::Android;
            #else
                    return Platform::Unspecified;
            #endif
        }
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILDINFO_H
