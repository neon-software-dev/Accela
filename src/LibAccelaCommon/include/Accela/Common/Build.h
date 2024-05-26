#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILD_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILD_H

namespace Accela::Common
{
    //
    // Enables cross-compiler usage of maybe_unused attribute
    //
    #ifdef __GNUC__
        #define SUPPRESS_IS_NOT_USED [[maybe_unused]]
    #else
        #define SUPPRESS_IS_NOT_USED
    #endif
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_BUILD_H
