cmake_minimum_required(VERSION 3.26.0)

include(SelectLibraryConfigurations)

set(PHYSX_VERSION "5.4.1")

if (NOT DEFINED PHYSX_INSTALL_DIR)
	set(PHYSX_INSTALL_DIR "${ACCELA_EXTERNAL_DIR}/PhysX-106.0-physx-5.4.1/physx/install")
endif()

if (WIN32)
	# Finds path to compiler-specific install directory, such as: vc17win64
	file(GLOB INSTALL_DIRS LIST_DIRECTORIES true "${PHYSX_INSTALL_DIR}/*")
	list(GET INSTALL_DIRS 0 INSTALL_DIRS_0)
	set(PHYSX_ROOT_DIR "${INSTALL_DIRS_0}/PhysX")

	# Finds path to compiler subdirectory, such as: win.x86_64.vc143.mt
	file(GLOB BIN_DIRS LIST_DIRECTORIES true "${PHYSX_ROOT_DIR}/bin/*")
	list(GET BIN_DIRS 0 BIN_DIR_0)

	set(PHYSX_BIN_DIR "${BIN_DIR_0}")
else()
	set(PHYSX_ROOT_DIR "${PHYSX_INSTALL_DIR}/linux/PhysX")

	# Finds path to compiler subdirectory, such as: linux.clang
	file(GLOB BIN_DIRS LIST_DIRECTORIES true "${PHYSX_ROOT_DIR}/bin/*")
	list(GET BIN_DIRS 0 BIN_DIR_0)

	set(PHYSX_BIN_DIR "${BIN_DIR_0}")
endif()

set(PHYSX_BIN_DEBUG_DIR "${PHYSX_BIN_DIR}/debug")
set(PHYSX_BIN_RELEASE_DIR "${PHYSX_BIN_DIR}/release")

find_path(PhysX_INCLUDE_DIRS NAMES PxPhysicsAPI.h HINTS "${PHYSX_ROOT_DIR}/include" REQUIRED)

function(FindPhysXLib LIBNAME ISREQUIRED)
	if(${ISREQUIRED})
		set(PHYSX_COMPONENT_REQUIRED "REQUIRED")
	endif()

	find_library(${LIBNAME}_LIBRARY_DEBUG NAMES ${LIBNAME}_64 ${LIBNAME}_static_64 HINTS ${PHYSX_BIN_DEBUG_DIR} ${PHYSX_COMPONENT_REQUIRED})
	find_library(${LIBNAME}_LIBRARY_RELEASE NAMES ${LIBNAME}_64 ${LIBNAME}_static_64 HINTS ${PHYSX_BIN_RELEASE_DIR} ${PHYSX_COMPONENT_REQUIRED})
	select_library_configurations(${LIBNAME})

	if(NOT "${${LIBNAME}_FOUND}")
		message(STATUS "PhysX - ${LIBNAME} not found")
		return()
	else()
		message(STATUS "PhysX - ${LIBNAME} found")
		set("${LIBNAME}_FOUND" TRUE PARENT_SCOPE)
	endif()

	if (${${LIBNAME}_FOUND} AND NOT ${LIBNAME})
		add_library(${LIBNAME} IMPORTED UNKNOWN)

		set(${LIBNAME}_INCLUDE_DIRS ${PHYSX_ROOT_DIR}/include)
		target_include_directories(${LIBNAME} INTERFACE ${${LIBNAME}_INCLUDE_DIRS})

		foreach(cfg IN ITEMS DEBUG RELEASE)
			if (${LIBNAME}_LIBRARY_${cfg})
				set_property(
					TARGET ${LIBNAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${cfg}
				)
				set_target_properties(
					${LIBNAME} PROPERTIES
					IMPORTED_LOCATION_${cfg} "${${LIBNAME}_LIBRARY_${cfg}}"
				)
			endif()
		endforeach()
	endif()

	# Export variables
	set(${LIBNAME}_VERSION "${PHYSX_VERSION}" PARENT_SCOPE)
endfunction()

FindPhysXLib("PhysXCharacterKinematic" true)
FindPhysXLib("PhysXCooking" true)
FindPhysXLib("PhysXExtensions" true)
FindPhysXLib("PhysX" true)
FindPhysXLib("PhysXPvdSDK" true)
FindPhysXLib("PhysXCommon" true)
FindPhysXLib("PhysXFoundation" true)
FindPhysXLib("PhysXGpu" false)

# Export variables
set(PhysX_FOUND TRUE PARENT_SCOPE)
set(PhysX_VERSION "${PHYSX_VERSION}" PARENT_SCOPE)
