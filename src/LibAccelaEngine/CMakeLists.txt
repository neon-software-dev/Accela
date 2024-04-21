cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaEngine VERSION 0.0.1 LANGUAGES CXX)

	find_package(EnTT CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)

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
	target_compile_options(AccelaEngine PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

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
		reactphysics3d
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
