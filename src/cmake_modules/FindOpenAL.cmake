cmake_minimum_required(VERSION 3.22)

set(OPENAL_VERSION "1.23.1")

if (NOT DEFINED OPENAL_INSTALL_DEBUG_DIR)
    set(OPENAL_INSTALL_DEBUG_DIR "${ACCELA_EXTERNAL_DIR}/build/debug")
endif()

if (NOT DEFINED OPENAL_INSTALL_RELEASE_DIR)
    set(OPENAL_INSTALL_RELEASE_DIR "${ACCELA_EXTERNAL_DIR}/build/release")
endif()

function(_OpenAL_find)
    include(FindPackageHandleStandardArgs)
    include(SelectLibraryConfigurations)

    ## Find the include path
    find_path(OpenAL_INCLUDE_DIR NAMES al.h HINTS "${OPENAL_INSTALL_RELEASE_DIR}/include/AL")

    ## Find the library
    find_library(OpenAL_LIBRARY_DEBUG NAMES openal openal32 HINTS "${OPENAL_INSTALL_DEBUG_DIR}/lib" REQUIRED)
    find_library(OpenAL_LIBRARY_RELEASE NAMES openal openal32 HINTS "${OPENAL_INSTALL_RELEASE_DIR}/lib" REQUIRED)

    select_library_configurations(OpenAL)

    if (OpenAL_FOUND AND NOT OpenAL::OpenAL)
        add_library(OpenAL::OpenAL IMPORTED UNKNOWN)

        set(OpenAL_INCLUDE_DIRS ${OpenAL_INCLUDE_DIR})
        target_include_directories(OpenAL::OpenAL INTERFACE ${OpenAL_INCLUDE_DIRS})

        foreach(cfg IN ITEMS DEBUG RELEASE)
            if (OpenAL_LIBRARY_${cfg})
                set_property(
                    TARGET OpenAL::OpenAL APPEND PROPERTY IMPORTED_CONFIGURATIONS ${cfg}
                )
                set_target_properties(
                    OpenAL::OpenAL PROPERTIES
                    IMPORTED_LOCATION_${cfg} "${OpenAL_LIBRARY_${cfg}}"
                )
            endif()
        endforeach()

        set(OpenAL_FOUND TRUE PARENT_SCOPE)
    endif()

    # Export variables
    set(OpenAL_VERSION "${OPENAL_VERSION}" PARENT_SCOPE)

endfunction()

_OpenAL_find()
