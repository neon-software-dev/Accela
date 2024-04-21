/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILDINFO_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILDINFO_H

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
    struct BuildInfo
    {
        /**
         * @return Whether or not the current build is a Debug build
         */
        static bool IsDebugBuild()
        {
            #if NDEBUG
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
            #if _WIN32 || _WIN64
                    return OS::Windows;
            #elif __linux__ || __unix__
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
