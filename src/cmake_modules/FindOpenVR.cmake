cmake_minimum_required(VERSION 3.22)

if (NOT DEFINED OPENVR_INSTALL_DEBUG_DIR)
  set(OPENVR_INSTALL_DEBUG_DIR "${ACCELA_EXTERNAL_DIR}/build/debug")
endif()

if (NOT DEFINED OPENVR_INSTALL_RELEASE_DIR)
  set(OPENVR_INSTALL_RELEASE_DIR "${ACCELA_EXTERNAL_DIR}/build/release")
endif()

function(_OpenVR_find)
  include(FindPackageHandleStandardArgs)
  include(SelectLibraryConfigurations)

  ## Find the include path
  find_path(OpenVR_INCLUDE_DIR NAMES openvr.h HINTS "${OPENVR_INSTALL_RELEASE_DIR}/include/openvr")

  ## Determine version by scanning header
  if (OpenVR_INCLUDE_DIR)
    set(openvr_header "${OpenVR_INCLUDE_DIR}/openvr.h")

    set(re_major "^\tstatic const uint32_t k_nSteamVRVersionMajor = ([0-9]+).*;$")
    set(re_minor "^\tstatic const uint32_t k_nSteamVRVersionMinor = ([0-9]+).*;$")
    set(re_patch "^\tstatic const uint32_t k_nSteamVRVersionBuild = ([0-9]+).*;$")

    file(STRINGS "${openvr_header}" OpenVR_VERSION_MAJOR REGEX "${re_major}")
    file(STRINGS "${openvr_header}" OpenVR_VERSION_MINOR REGEX "${re_minor}")
    file(STRINGS "${openvr_header}" OpenVR_VERSION_PATCH REGEX "${re_patch}")

    string(REGEX REPLACE "${re_major}" "\\1"
           OpenVR_VERSION_MAJOR "${OpenVR_VERSION_MAJOR}")
    string(REGEX REPLACE "${re_minor}" "\\1"
           OpenVR_VERSION_MINOR "${OpenVR_VERSION_MINOR}")
    string(REGEX REPLACE "${re_patch}" "\\1"
           OpenVR_VERSION_PATCH "${OpenVR_VERSION_PATCH}")

    if (OpenVR_VERSION_MAJOR AND OpenVR_VERSION_MINOR AND OpenVR_VERSION_PATCH)
      set(OpenVR_VERSION
          "${OpenVR_VERSION_MAJOR}.${OpenVR_VERSION_MINOR}.${OpenVR_VERSION_PATCH}")
    endif ()
  endif ()

  ## Find the library
  find_library(OpenVR_LIBRARY_DEBUG NAMES openvr_api openvr_api64 HINTS "${OPENVR_INSTALL_DEBUG_DIR}/lib" REQUIRED)
  find_library(OpenVR_LIBRARY_RELEASE NAMES openvr_api openvr_api64 HINTS "${OPENVR_INSTALL_RELEASE_DIR}/lib" REQUIRED)

  select_library_configurations(OpenVR)

  ## Perform all the standard required, version, etc. argument checks.
  find_package_handle_standard_args(
    OpenVR
      REQUIRED_VARS OpenVR_LIBRARY OpenVR_INCLUDE_DIR
      VERSION_VAR OpenVR_VERSION
      HANDLE_VERSION_RANGE
      HANDLE_COMPONENTS
  )

  ## Create OpenVR::OpenVR imported target.
  if (OpenVR_FOUND AND NOT OpenVR::OpenVR)
    add_library(OpenVR::OpenVR UNKNOWN IMPORTED)
    target_include_directories(OpenVR::OpenVR INTERFACE "${OpenVR_INCLUDE_DIR}")
    set_target_properties(
      OpenVR::OpenVR PROPERTIES IMPORTED_LOCATION "${OpenVR_LIBRARY}"
    )

    foreach (cfg IN ITEMS RELEASE DEBUG)
      if (OpenVR_LIBRARY_${cfg})
        set_property(
          TARGET OpenVR::OpenVR APPEND PROPERTY IMPORTED_CONFIGURATIONS ${cfg}
        )
        set_target_properties(
          OpenVR::OpenVR PROPERTIES
          IMPORTED_LOCATION_${cfg} "${OpenVR_LIBRARY_${cfg}}"
        )
      endif ()
    endforeach ()

    set(OpenVR_FOUND TRUE PARENT_SCOPE)
  endif ()

  # Export variables
  set(OpenVR_VERSION "${OpenVR_VERSION}" PARENT_SCOPE)

endfunction()

_OpenVR_find()
