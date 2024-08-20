cmake_minimum_required(VERSION 3.26.0)

project(Accela VERSION 0.0.1)


####
# Global variables
####

set(CMAKE_DEBUG_POSTFIX d)

# TODO! <0:>
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/built/bin)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_INSTALL_RPATH $ORIGIN)

set(ACCELA_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)
set(EXTERNAL_BINARIES_DIR_DEBUG "${ACCELA_EXTERNAL_DIR}/build/debug/bin")
set(EXTERNAL_BINARIES_DIR_RELEASE "${ACCELA_EXTERNAL_DIR}/build/release/bin")

set(VCPKG_BINARIES_DIR_DEBUG "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
set(VCPKG_BINARIES_DIR_RELEASE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(ACCELA_WARNINGS_FLAGS
        -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wformat=2 -Wlogical-op
        -Wmissing-include-dirs -Wpointer-arith -Wredundant-decls -Wsequence-point
        -Wshadow -Wswitch -Wunreachable-code -Wunused-but-set-parameter
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # TODO: Test building with clang and add in all the clang error flags
    set(ACCELA_WARNINGS_FLAGS
        -Wall -Wextra -Werror
    )
endif()

####
# Append custom cmake files from cmake_modules dir
####

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

####
# Add projects
####

add_subdirectory(AccelaEngine)
add_subdirectory(AccelaEditor)
add_subdirectory(TestDesktopApp)

#[===[
# Exports

####
# Install non-Accela *runtime* dependencies

# vcpkg-provided runtime dependencies
if(EXISTS ${VCPKG_BINARIES_DIR_DEBUG} OR EXISTS ${VCPKG_BINARIES_DIR_RELEASE})
    install(
        DIRECTORY $<IF:$<CONFIG:Debug>,${VCPKG_BINARIES_DIR_DEBUG}/,${VCPKG_BINARIES_DIR_RELEASE}/>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# externally built runtime dependencies
if(EXISTS ${EXTERNAL_BINARIES_DIR_DEBUG} OR EXISTS ${EXTERNAL_BINARIES_DIR_RELEASE})
    install(
        DIRECTORY $<IF:$<CONFIG:Debug>,${EXTERNAL_BINARIES_DIR_DEBUG}/,${EXTERNAL_BINARIES_DIR_RELEASE}/>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

]===]
