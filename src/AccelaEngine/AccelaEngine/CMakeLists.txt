cmake_minimum_required(VERSION 3.26.0)

project(AccelaEngine VERSION 0.0.1 LANGUAGES CXX)

	find_package(assimp CONFIG REQUIRED)
	find_package(EnTT CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(CUDAToolkit REQUIRED)
	find_package(PhysX REQUIRED)
	find_package(OpenAL CONFIG REQUIRED)
	find_package(FFMPEG REQUIRED)

	# PhysX installs a Gpu shared lib on linux but not windows. Link against it on linux and not on windows.
	if (PhysXGpu_FOUND)
		set(PhysXGpuOptional "PhysXGpu")
	endif()

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
	file(GLOB AccelaEngine_Include_Headers_Media "include/Accela/Engine/Media/*.h")

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
	file(GLOB AccelaEngine_Sources_Media "src/Media/*.cpp")
	file(GLOB AccelaEngine_Headers_Media "src/Media/*.h")
	file(GLOB AccelaEngine_Sources_Media_FFMPEG "src/Media/FFMPEG/*.cpp")
	file(GLOB AccelaEngine_Headers_Media_FFMPEG "src/Media/FFMPEG/*.h")

add_library(AccelaEngine
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
	${AccelaEngine_Include_Headers_Media}

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
	${AccelaEngine_Sources_Media}
	${AccelaEngine_Headers_Media}
	${AccelaEngine_Sources_Media_FFMPEG}
	${AccelaEngine_Headers_Media_FFMPEG}
)

target_link_directories(AccelaEngine PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(AccelaEngine
	PUBLIC
		AccelaCommon
		AccelaRenderer
		AccelaPlatform
	PRIVATE
		assimp::assimp
		EnTT::EnTT
		OpenAL::OpenAL
		AudioFile
		${FFMPEG_LIBRARIES}
		PhysXCharacterKinematic
		PhysXExtensions
		PhysX
		PhysXPvdSDK
		${PhysXGpuOptional}
		PhysXCooking
		PhysXCommon
		PhysXFoundation
		CUDA::cuda_driver
)

target_compile_features(AccelaEngine PUBLIC cxx_std_23 PRIVATE cxx_std_23)

target_compile_options(AccelaEngine PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_include_directories(AccelaEngine
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${FFMPEG_INCLUDE_DIRS}
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_INCLUDEDIR})

#[===[
if (PhysXGpu_FOUND)
	install(
		FILES
			$<IF:$<CONFIG:Debug>,${PhysXGpu_LIBRARY_DEBUG},${PhysXGpu_LIBRARY_RELEASE}>
		DESTINATION "${CMAKE_INSTALL_BINDIR}"
	)
endif()

# Install OpenAL static lib
install(
	FILES
		$<IF:$<CONFIG:Debug>,${OpenAL_LIBRARY_DEBUG},${OpenAL_LIBRARY_RELEASE}>
	DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Install external static libs, if building static
if (ACCELA_STATIC)
	# Install PhysX static lib
	install(
		FILES
			$<IF:$<CONFIG:Debug>,${PhysXCharacterKinematic_LIBRARY_DEBUG},${PhysXCharacterKinematic_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysXCooking_LIBRARY_DEBUG},${PhysXCooking_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysXExtensions_LIBRARY_DEBUG},${PhysXExtensions_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysX_LIBRARY_DEBUG},${PhysX_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysXPvdSDK_LIBRARY_DEBUG},${PhysXPvdSDK_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysXCommon_LIBRARY_DEBUG},${PhysXCommon_LIBRARY_RELEASE}>
			$<IF:$<CONFIG:Debug>,${PhysXFoundation_LIBRARY_DEBUG},${PhysXFoundation_LIBRARY_RELEASE}>
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	)

	if (PhysXGpu_FOUND)
		install(
			FILES
				$<IF:$<CONFIG:Debug>,${PhysXGpu_LIBRARY_DEBUG},${PhysXGpu_LIBRARY_RELEASE}>
			DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()

	# Install OpenAL static lib
	install(
		FILES
			$<IF:$<CONFIG:Debug>,${OpenAL_LIBRARY_DEBUG},${OpenAL_LIBRARY_RELEASE}>
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	)
endif()

]===]
