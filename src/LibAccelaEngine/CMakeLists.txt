cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaEngine VERSION 0.0.1 LANGUAGES CXX)

	find_package(EnTT CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)

	find_library(PhysXCharacterKinematic PhysXCharacterKinematic_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysXCooking PhysXCooking_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysXExtensions PhysXExtensions_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysX PhysX_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysXPvdSDK PhysXPvdSDK_static_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysXGpu PhysXGpu_64 HINTS "${PHYSX_BIN_DIR}") # TODO! Windows: PhysX doesn't build PhysXGpu? Why?
	find_library(PhysXCommon PhysXCommon_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	find_library(PhysXFoundation PhysXFoundation_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)
	#find_library(PVDRuntime PVDRuntime_64 HINTS "${PHYSX_BIN_DIR}" REQUIRED)

	set (PHYSX_LIBRARIES
		${PhysXCharacterKinematic}
		${PhysXCooking}
		${PhysXExtensions}
		${PhysX}
		${PhysXPvdSDK}
		${PhysXCommon}
		${PhysXFoundation}
		#${PVDRuntime}
	)

	file(GLOB AccelaEngine_Include_Headers "include/Accela/Engine/*.h")
	file(GLOB AccelaEngine_Include_Headers_Scene "include/Accela/Engine/Scene/*.h")
	file(GLOB AccelaEngine_Include_Headers_Component "include/Accela/Engine/Component/*.h")
	file(GLOB AccelaEngine_Include_Headers_Audio "include/Accela/Engine/Audio/*.h")
	file(GLOB AccelaEngine_Include_Headers_Entity "include/Accela/Engine/Entity/*.h")
	file(GLOB AccelaEngine_Include_Headers_Model "include/Accela/Engine/Model/*.h")
	file(GLOB AccelaEngine_Include_Headers_Bounds "include/Accela/Engine/Bounds/*.h")
	file(GLOB AccelaEngine_Include_Headers_Physics "include/Accela/Engine/Physics/*.h")

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

add_library(AccelaEngine STATIC
	${AccelaEngine_Include_Headers}
	${AccelaEngine_Include_Headers_Scene}
	${AccelaEngine_Include_Headers_Component}
	${AccelaEngine_Include_Headers_Audio}
	${AccelaEngine_Include_Headers_Entity}
	${AccelaEngine_Include_Headers_Model}
	${AccelaEngine_Include_Headers_Bounds}
	${AccelaEngine_Include_Headers_Physics}

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
)

target_compile_features(AccelaEngine
	PUBLIC
		cxx_std_23
)

if (NOT MSVC)
	target_compile_options(AccelaEngine PRIVATE ${ACCELA_WARNINGS_FLAGS})
endif()

#########################
# TODO! Remove when this block when PhysX package in vcpkg isn't broken for linux
target_include_directories(AccelaEngine
	PRIVATE
		"${PHYSX_INSTALL_DIR}/include"
)
#if(CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#	add_compile_definitions(_DEBUG)
#else()
#	add_compile_definitions(NDEBUG)
#endif()
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
		${PHYSX_LIBRARIES}
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
