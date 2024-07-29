cmake_minimum_required(VERSION 3.26.0)

project(LibAccelaEngine VERSION 0.0.1 LANGUAGES CXX)

	# TODO: Eventually default to ON when cuda-integration is better supported
	option(ACCELA_USE_GPU_CUDA "Attempt to use CUDA/GPU for physics computations" OFF)

	find_package(EnTT CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)
	find_package(CUDAToolkit REQUIRED)

	#########################
	# TODO: Remove this block when PhysX package in vcpkg isn't broken for linux
	find_library(PhysXCharacterKinematicFile PhysXCharacterKinematic_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCharacterKinematic IMPORTED STATIC)
	set_target_properties(PhysXCharacterKinematic PROPERTIES
		IMPORTED_LOCATION "${PhysXCharacterKinematicFile}"
	)

	find_library(PhysXCookingFile PhysXCooking_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCooking IMPORTED STATIC)
	set_target_properties(PhysXCooking PROPERTIES
		IMPORTED_LOCATION "${PhysXCookingFile}"
	)

	find_library(PhysXExtensionsFile PhysXExtensions_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXExtensions IMPORTED STATIC)
	set_target_properties(PhysXExtensions PROPERTIES
		IMPORTED_LOCATION "${PhysXExtensionsFile}"
	)

	find_library(PhysXFile PhysX_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysX IMPORTED STATIC)
	set_target_properties(PhysX PROPERTIES
		IMPORTED_LOCATION "${PhysXFile}"
	)

	find_library(PhysXPvdSDKFile PhysXPvdSDK_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXPvdSDK IMPORTED STATIC)
	set_target_properties(PhysXPvdSDK PROPERTIES
		IMPORTED_LOCATION "${PhysXPvdSDKFile}"
	)

	find_library(PhysXCommonFile PhysXCommon_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXCommon IMPORTED STATIC)
	set_target_properties(PhysXCommon PROPERTIES
		IMPORTED_LOCATION "${PhysXCommonFile}"
	)

	find_library(PhysXFoundationFile PhysXFoundation_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	add_library(PhysXFoundation IMPORTED STATIC)
	set_target_properties(PhysXFoundation PROPERTIES
		IMPORTED_LOCATION "${PhysXFoundationFile}"
	)

	find_library(PhysXGpuFile PhysXGpu_64 HINTS "${PHYSX_BIN_DIR}")
	add_library(PhysXGpu IMPORTED SHARED)
	set_target_properties(PhysXGpu PROPERTIES
		IMPORTED_LOCATION "${PhysXGpuFile}"
		IMPORTED_IMPLIB "${PhysXGpuFile}"
	)

	# PhysX installs a Gpu lib on linux but not windows. Link against it on linux and not on windows.
	# PhysXGpuFile is also not marked as REQUIRED above.
	if (UNIX)
		set(PhysXGpuFileOptional PhysXGpuFile)
		set(PhysXGpuOptional PhysXGpu)
	endif()

	set(PHYSX_TARGETS
		PhysXCharacterKinematic
		PhysXExtensions
		PhysX
		PhysXPvdSDK
		${PhysXGpuOptional}
		PhysXCooking
		PhysXCommon
		PhysXFoundation
	)

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
	file(GLOB AccelaEngine_Include_Headers_Extra "include/Accela/Engine/Extra/*.h")

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
	file(GLOB AccelaEngine_Sources_Extra "src/Extra/*.cpp")
	file(GLOB AccelaEngine_Headers_Extra "src/Extra/*.h")

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
	${AccelaEngine_Include_Headers_Extra}

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
	${AccelaEngine_Sources_Extra}
	${AccelaEngine_Headers_Extra}
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
		CUDA::cuda_driver # TODO: Verify this links on windows; it isn't needed there but is on linux
)

target_include_directories(AccelaEngine
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install PhysX dynamic libraries
if (UNIX)
	install(IMPORTED_RUNTIME_ARTIFACTS PhysXGpu)
endif ()

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
