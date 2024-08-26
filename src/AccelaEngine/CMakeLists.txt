cmake_minimum_required(VERSION 3.26.0)

project(AccelaEngine VERSION 0.0.1)

####
# Global options
####

option(ACCELA_STATIC "Build Accela as a static library" OFF)
option(ACCELA_USE_GPU_CUDA "Use GPU/CUDA for physics computations" OFF)

####
# Other config
####

# Set ACCELA_TARGET_PLATFORM to either Desktop or Android to build for the desired platform.
# Will default to Desktop if unset.

# Set ACCELAENGINE_INSTALL_SUBDIR to have the engine install itself to a specific subdirectory
# within CMake's install prefix directory.

####
# Global constants
####

set(ACCELA_TARGET_PLATFORM_DESKTOP "Desktop")
set(ACCELA_TARGET_PLATFORM_ANDROID "Android")

####
# Global variables
####

# Default target platform to desktop, if not specified
if (NOT DEFINED ACCELA_TARGET_PLATFORM)
   set(ACCELA_TARGET_PLATFORM ${ACCELA_TARGET_PLATFORM_DESKTOP})
endif()

####
# Global compile definitions
####

add_compile_definitions(
    ACCELA_DO_EXPORT    # Configure SharedLib.h to mark ACCELA_PUBLIC as an export
)

if (ACCELA_STATIC)
    add_compile_definitions(ACCELA_STATIC)
endif()

####
# Add internal subprojects
####

# Configure BUILD_SHARED_LIBS for internal subprojects
if (ACCELA_STATIC)
    message(STATUS "ACCELA: Configured as static library")
    set(BUILD_SHARED_LIBS FALSE)
else()
    message(STATUS "ACCELA: Configured as dynamic library")
    set(BUILD_SHARED_LIBS TRUE)
endif()

# Add platform-independent projects
add_subdirectory(AccelaCommon)
add_subdirectory(AccelaPlatform)
add_subdirectory(AccelaRenderer)
add_subdirectory(AccelaEngine)

# Add platform-specific projects
message(STATUS "ACCELA: Configured for target platform: ${ACCELA_TARGET_PLATFORM}")

if (ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_DESKTOP})
    add_subdirectory(AccelaPlatformDesktop)
    add_subdirectory(AccelaPlatformDesktopSDL)
    add_subdirectory(AccelaPlatformDesktopQt)
    add_subdirectory(AccelaRendererVk)
    add_subdirectory(AccelaEngineDesktop)
elseif(ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_ANDROID})
    # no-op
else()
    message(FATAL_ERROR "ACCELA: Unsupported target platform, configure with -DACCELA_TARGET_PLATFORM=[Desktop/Android]")
endif()

####
# AccelaEngine Installation
####

set(ACCELAENGINE_NAMESPACE            "Accela::")
set(ACCELAENGINE_PACKAGENAME          "AccelaEngine")
set(ACCELAENGINE_GENERATED_DIR        "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ACCELAENGINE_VERSION_CONFIG_FILE  "${ACCELAENGINE_GENERATED_DIR}/${ACCELAENGINE_PACKAGENAME}ConfigVersion.cmake")
set(ACCELAENGINE_PROJECT_CONFIG_FILE  "${ACCELAENGINE_GENERATED_DIR}/${ACCELAENGINE_PACKAGENAME}Config.cmake")
set(ACCELAENGINE_CONFIG_INSTALL_DIR   "${ACCELAENGINE_INSTALL_SUBDIR}cmake/${ACCELAENGINE_PACKAGENAME}")
set(ACCELAENGINE_TARGETS_EXPORT_NAME  "${ACCELAENGINE_PACKAGENAME}Targets")

# Install Accela platform-independent targets
install(
    TARGETS
        AccelaCommon
        AccelaPlatform
        AccelaRenderer
        AccelaEngine
    EXPORT ${ACCELAENGINE_TARGETS_EXPORT_NAME}
    LIBRARY DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_BINDIR}
)

# Install Accela platform-specific targets
if (ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_DESKTOP})
    install(
        TARGETS
            AccelaPlatformDesktop
            AccelaPlatformDesktopSDL
            AccelaPlatformDesktopQt
            AccelaRendererVk
            AccelaEngineDesktop
        EXPORT ${ACCELAENGINE_TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_BINDIR}
    )
elseif(ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_ANDROID})
    # no-op
endif()

# Generate and install cmake config files
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${ACCELAENGINE_VERSION_CONFIG_FILE}"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in" "${ACCELAENGINE_PROJECT_CONFIG_FILE}" @ONLY)

install(
    FILES "${ACCELAENGINE_VERSION_CONFIG_FILE}" "${ACCELAENGINE_PROJECT_CONFIG_FILE}"
    DESTINATION "${ACCELAENGINE_CONFIG_INSTALL_DIR}"
)

install(
    EXPORT "${ACCELAENGINE_TARGETS_EXPORT_NAME}"
    NAMESPACE "${ACCELAENGINE_NAMESPACE}"
    DESTINATION "${ACCELAENGINE_CONFIG_INSTALL_DIR}"
)
