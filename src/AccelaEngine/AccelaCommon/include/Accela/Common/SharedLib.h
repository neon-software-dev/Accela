/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_SHAREDLIB_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_SHAREDLIB_H

#ifdef ACCELA_STATIC
    #define ACCELA_PUBLIC
    #define ACCELA_LOCAL
#else
    #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        #ifdef ACCELA_DO_EXPORT
            #ifdef __GNUC__
                #define ACCELA_PUBLIC __attribute__ ((dllexport))
            #else
                #define ACCELA_PUBLIC __declspec(dllexport)
            #endif
        #else
            #ifdef __GNUC__
                #define ACCELA_PUBLIC __attribute__ ((dllimport))
            #else
                #define ACCELA_PUBLIC __declspec(dllimport)
            #endif
        #endif

        #define ACCELA_LOCAL
    #else
        #if __GNUC__ >= 4
            #define ACCELA_PUBLIC __attribute__ ((visibility ("default")))
            #define ACCELA_LOCAL  __attribute__ ((visibility ("hidden")))
        #else
            #define ACCELA_PUBLIC
            #define ACCELA_LOCAL
        #endif
    #endif
#endif

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_SHAREDLIB_H
