cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaEngine VERSION 0.0.1 LANGUAGES CXX)

	# TODO: Eventually default to ON when cuda-integration is better supported
	option(ACCELA_USE_GPU_CUDA "Attempt to use CUDA/GPU for physics computations" OFF)

	find_package(EnTT CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)

	#########################
	# TODO: Remove this block when PhysX package in vcpkg isn't broken for linux
	find_library(PhysXCharacterKinematicFile PhysXCharacterKinematic_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCharacterKinematic IMPORTED STATIC)
	set_target_properties(PhysXCharacterKinematic PROPERTIES
		IMPORTED_LOCATION "${PhysXCharacterKinematicFile}"
	)

	find_library(PhysXCookingFile PhysXCooking_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCooking IMPORTED SHARED)
	set_target_properties(PhysXCooking PROPERTIES
		IMPORTED_LOCATION "${PhysXCookingFile}"
		IMPORTED_IMPLIB "${PhysXCookingFile}"
	)

	find_library(PhysXExtensionsFile PhysXExtensions_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXExtensions IMPORTED STATIC)
	set_target_properties(PhysXExtensions PROPERTIES
		IMPORTED_LOCATION "${PhysXExtensionsFile}"
	)

	find_library(PhysXFile PhysX_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysX IMPORTED SHARED)
	set_target_properties(PhysX PROPERTIES
		IMPORTED_LOCATION "${PhysXFile}"
		IMPORTED_IMPLIB "${PhysXFile}"
	)

	find_library(PhysXPvdSDKFile PhysXPvdSDK_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXPvdSDK IMPORTED STATIC)
	set_target_properties(PhysXPvdSDK PROPERTIES
		IMPORTED_LOCATION "${PhysXPvdSDKFile}"
	)

	find_library(PhysXCommonFile PhysXCommon_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCommon IMPORTED SHARED)
	set_target_properties(PhysXCommon PROPERTIES
		IMPORTED_LOCATION "${PhysXCommonFile}"
		IMPORTED_IMPLIB "${PhysXCommonFile}"
	)

	find_library(PhysXFoundationFile PhysXFoundation_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXFoundation IMPORTED SHARED)
	set_target_properties(PhysXFoundation PROPERTIES
		IMPORTED_LOCATION "${PhysXFoundationFile}"
		IMPORTED_IMPLIB "${PhysXFoundationFile}"
	)

	set(PHYSX_TARGETS
		PhysXCharacterKinematic
		PhysXCooking
		PhysXExtensions
		PhysX
		PhysXPvdSDK
		PhysXCommon
		PhysXFoundation
	)

	set_target_properties(${PHYSX_TARGETS} PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
	)

	if (WIN32)
		set (PHYSX_WINDOWS_DLLS
			"${PHYSX_BIN_DIR}/PhysXCooking_64.dll"
			"${PHYSX_BIN_DIR}/PhysX_64.dll"
			"${PHYSX_BIN_DIR}/PhysXCommon_64.dll"
			"${PHYSX_BIN_DIR}/PhysXFoundation_64.dll"
		)
	endif ()

	#
	#########################

	file(GLOB AccelaEngine_Include_Headers "include/Accela/Engine/*.h")
	file(GLOB AccelaEngine_Include_Headers_Scene "include/Accela/Engine/Scene/*.h")
	file(GLOB AccelaEngine_Include_Headers_Component "include/Accela/Engine/Component/*.h")
	file(GLOB AccelaEngine_Include_Headers_Audio "include/Accela/Engine/Audio/*.h")
	file(GLOB AccelaEngine_Include_Headers_Entity "include/Accela/Engine/Entity/*.h")
	file(GLOB AccelaEngine_Include_Headers_Model "include/Accela/Engine/Model/*.h")
	file(GLOB AccelaEngine_Include_Headers_Bounds "include/Accela/Engine/Bounds/*.h")
	file(GLOB AccelaEngine_Include_Headers_Physics "include/Accela/Engine/Physics/*.h")
	file(GLOB AccelaEngine_Include_Headers_Util "include/Accela/Engine/Util/*.h")
	file(GLOB AccelaEngine_Include_Headers_Material "include/Accela/Engine/Material/*.h")
	file(GLOB AccelaEngine_Include_Headers_Package "include/Accela/Engine/Package/*.h")

	file(GLOB AccelaEngine_Sources "src/*.cpp")
	file(GLOB AccelaEngine_Headers "src/*.h")
	file(GLOB AccelaEngine_Sources_Component "src/Component/*.cpp")
	file(GLOB AccelaEngine_Headers_Component "src/Component/*.h")
	file(GLOB AccelaEngine_Sources_Scene "src/Scene/*.cpp")
	file(GLOB AccelaEngine_Headers_Scene "src/Scene/*.h")
	file(GLOB AccelaEngine_Sources_Audio "src/Audio/*.cpp")
	file(GLOB AccelaEngine_Headers_Audio "src/Audio/*.h")
	file(GLOB AccelaEngine_Sources_Entity "src/Entity/*.cpp")
	file(GLOB AccelaEngine_Headers_Entity "src/Entity/*.h")
	file(GLOB AccelaEngine_Sources_Model "src/Model/*.cpp")
	file(GLOB AccelaEngine_Headers_Model "src/Model/*.h")
	file(GLOB AccelaEngine_Sources_Texture "src/Texture/*.cpp")
	file(GLOB AccelaEngine_Headers_Texture "src/Texture/*.h")
	file(GLOB AccelaEngine_Sources_Util "src/Util/*.cpp")
	file(GLOB AccelaEngine_Headers_Util "src/Util/*.h")
	file(GLOB AccelaEngine_Sources_Physics "src/Physics/*.cpp")
	file(GLOB AccelaEngine_Headers_Physics "src/Physics/*.h")
	file(GLOB AccelaEngine_Sources_Package "src/Package/*.cpp")
	file(GLOB AccelaEngine_Headers_Package "src/Package/*.h")

add_library(AccelaEngine STATIC
	${AccelaEngine_Include_Headers}
	${AccelaEngine_Include_Headers_Scene}
	${AccelaEngine_Include_Headers_Component}
	${AccelaEngine_Include_Headers_Audio}
	${AccelaEngine_Include_Headers_Entity}
	${AccelaEngine_Include_Headers_Model}
	${AccelaEngine_Include_Headers_Bounds}
	${AccelaEngine_Include_Headers_Physics}
	${AccelaEngine_Include_Headers_Util}
	${AccelaEngine_Include_Headers_Material}
	${AccelaEngine_Include_Headers_Package}

	${AccelaEngine_Sources}
	${AccelaEngine_Headers}
	${AccelaEngine_Sources_Component}
	${AccelaEngine_Headers_Component}
	${AccelaEngine_Sources_Scene}
	${AccelaEngine_Headers_Scene}
	${AccelaEngine_Sources_Audio}
	${AccelaEngine_Headers_Audio}
	${AccelaEngine_Sources_Entity}
	${AccelaEngine_Headers_Entity}
	${AccelaEngine_Sources_Model}
	${AccelaEngine_Headers_Model}
	${AccelaEngine_Sources_Texture}
	${AccelaEngine_Headers_Texture}
	${AccelaEngine_Sources_Util}
	${AccelaEngine_Headers_Util}
	${AccelaEngine_Sources_Physics}
	${AccelaEngine_Headers_Physics}
	${AccelaEngine_Sources_Package}
	${AccelaEngine_Headers_Package}
)

if (ACCELA_USE_GPU_CUDA)
	target_compile_definitions(AccelaEngine PRIVATE "ACCELA_USE_GPU_CUDA")
endif()

target_compile_features(AccelaEngine PUBLIC cxx_std_23)

target_compile_options(AccelaEngine PRIVATE ${ACCELA_WARNINGS_FLAGS})

#########################
# TODO: Remove when this block when PhysX package in vcpkg isn't broken for linux
target_include_directories(AccelaEngine
	PRIVATE
		"${PHYSX_INSTALL_DIR}/include"
)
#
#########################

target_link_libraries(AccelaEngine
	PUBLIC
		AccelaCommon
		AccelaRenderer
		AccelaPlatform
	PRIVATE
		EnTT::EnTT
		OpenAL::OpenAL
		AudioFile
		assimp::assimp
		${PHYSX_TARGETS}
)

target_include_directories(AccelaEngine
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

####
# Post-Build
####

# So that TestDesktopApp can run, on Windows, copy PhysX dlls to the runtime
# dir as it needs them in the same dir to run. On Linux, the TestDesktopApp
# executable is internally linked to the dlls in build hierarchy
if (WIN32)
	add_custom_command(TARGET AccelaEngine POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PHYSX_WINDOWS_DLLS}
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	)
endif()

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install PhysX dynamic libraries
install(IMPORTED_RUNTIME_ARTIFACTS PhysXCooking PhysX PhysXCommon PhysXFoundation)

# Install PhysX libraries
install(
	FILES ${PhysXCharacterKinematicFile} ${PhysXCookingFile} ${PhysXExtensionsFile} ${PhysXFile} ${PhysXPvdSDKFile} ${PhysXCommonFile} ${PhysXFoundationFile}
	DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# On Windows, manually install PhysX dlls
if (WIN32)
	install(
		FILES ${PHYSX_WINDOWS_DLLS}
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	)
endif()
