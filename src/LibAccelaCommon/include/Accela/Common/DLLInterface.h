#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_DLLINTERFACE_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_DLLINTERFACE_H

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #ifdef ACCELA_DLL_EXPORT
        #ifdef __GNUC__
            #define DLL_PUBLIC __attribute__ ((dllexport))
        #else
            #define DLL_PUBLIC __declspec(dllexport)
        #endif
    #else
        #ifdef __GNUC__
            #define DLL_PUBLIC __attribute__ ((dllimport))
        #else
            #define DLL_PUBLIC __declspec(dllimport)
        #endif
    #endif

    #define DLL_LOCAL
#else
    #if __GNUC__ >= 4
        #define DLL_PUBLIC __attribute__ ((visibility ("default")))
        #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
        #define DLL_PUBLIC
        #define DLL_LOCAL
    #endif

#endif

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_DLLINTERFACE_H
