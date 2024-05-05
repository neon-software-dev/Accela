cmake_minimum_required(VERSION 3.19.0)

project(Accela VERSION 0.0.1)

####
# Global set configuration
####

set(ACCELA_TARGET_PLATFORM_DESKTOP "Desktop")
set(ACCELA_TARGET_PLATFORM_ANDROID "Android")

set(CMAKE_DEBUG_POSTFIX d)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # TODO: Also add -Wfloat-equal and address issues
    set(ACCELA_WARNINGS_FLAGS -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wpointer-arith -Wredundant-decls -Wsequence-point -Wshadow -Wswitch -Wunreachable-code -Wunused-but-set-parameter)
endif()

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built)

####
# Global compile definitions
####

add_compile_definitions(ACCELA_DLL_EXPORT)

if (CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(_DEBUG)
else()
	add_compile_definitions(NDEBUG)
endif()

####
# Append custom cmake files from cmake_modules dir
####

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

####
# Add external subprojects
####

set(ACCELA_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)

# External OpenAL lib
set(ALSOFT_UTILS OFF)
set(ALSOFT_NO_CONFIG_UTIL ON)
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_TESTS OFF)
set(ALSOFT_INSTALL_EXAMPLES OFF)
set(ALSOFT_INSTALL_UTILS OFF)
add_subdirectory(External/openal-soft)

####
# Configure internal subprojects
####
if (WIN32)
    # We're intentionally defining these after building external libs above
    # so as to not affect their builds
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

####
# Add internal subprojects
####
add_subdirectory(LibAccelaCommon)
add_subdirectory(LibAccelaEngine)
add_subdirectory(LibAccelaPlatform)
add_subdirectory(LibAccelaRenderer)

message(STATUS "ACCELA: Configuring for target platform: ${ACCELA_TARGET_PLATFORM}")

if (ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_DESKTOP})
    add_subdirectory(LibAccelaEngineDesktop)
    add_subdirectory(LibAccelaPlatformDesktop)
    add_subdirectory(LibAccelaRendererVk)
    add_subdirectory(TestDesktopApp)

elseif(ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_ANDROID})
    # no-op
else()
    message(FATAL_ERROR "ACCELA: Unsupported target platform, configure with -DACCELA_TARGET_PLATFORM=[Desktop/Android]")

endif()

####
# Installation
####

set(ACCELA_NAMESPACE            "${PROJECT_NAME}::")
set(ACCELA_GENERATED_DIR        "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ACCELA_VERSION_CONFIG_FILE  "${ACCELA_GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(ACCELA_PROJECT_CONFIG_FILE  "${ACCELA_GENERATED_DIR}/${PROJECT_NAME}Config.cmake")
set(ACCELA_CONFIG_INSTALL_DIR   "lib/cmake/${PROJECT_NAME}")
set(ACCELA_TARGETS_EXPORT_NAME  "${PROJECT_NAME}Targets")

# Exports
install(
    TARGETS
        AccelaCommon
        AccelaEngine
        AccelaPlatform
        AccelaRenderer
    EXPORT ${ACCELA_TARGETS_EXPORT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_DESKTOP})

install(
    TARGETS
        AccelaEngineDesktop
        AccelaPlatformDesktop
        AccelaRendererVk
    EXPORT ${ACCELA_TARGETS_EXPORT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

elseif(ACCELA_TARGET_PLATFORM STREQUAL ${ACCELA_TARGET_PLATFORM_ANDROID})
    # no-op
endif()

# Generate config version file
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${ACCELA_VERSION_CONFIG_FILE}" VERSION ${CMAKE_PROJECT_VERSION} COMPATIBILITY SameMajorVersion
)

# Configure config.cmake.in
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/Config.cmake.in" "${ACCELA_PROJECT_CONFIG_FILE}" @ONLY)

# Install cmake config files
install(
    FILES "${ACCELA_VERSION_CONFIG_FILE}" "${ACCELA_PROJECT_CONFIG_FILE}"
    DESTINATION "${ACCELA_CONFIG_INSTALL_DIR}"
)

# Install cmake target files
install(
    EXPORT "${ACCELA_TARGETS_EXPORT_NAME}" NAMESPACE "${ACCELA_NAMESPACE}"
    DESTINATION "${ACCELA_CONFIG_INSTALL_DIR}"
)
