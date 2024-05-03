cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaRenderer VERSION 0.0.1 LANGUAGES CXX)

	find_package(glm CONFIG REQUIRED)

	file(GLOB AccelaRenderer_Include_Headers "include/Accela/Render/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Task "include/Accela/Render/Task/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Texture "include/Accela/Render/Texture/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Graph "include/Accela/Render/Graph/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Renderable "include/Accela/Render/Renderable/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Shader "include/Accela/Render/Shader/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Util "include/Accela/Render/Util/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Mesh "include/Accela/Render/Mesh/*.h")
	file(GLOB AccelaRenderer_Include_Headers_Material "include/Accela/Render/Material/*.h")

	file(GLOB AccelaRenderer_Sources "src/*.cpp")
	file(GLOB AccelaRenderer_Headers "src/*.h")
	file(GLOB AccelaRenderer_Sources_Task "src/Task/*.cpp")
	file(GLOB AccelaRenderer_Headers_Task "src/Task/*.h")
	file(GLOB AccelaRenderer_Sources_Util "src/Util/*.cpp")
	file(GLOB AccelaRenderer_Headers_Util "src/Util/*.h")

add_library(AccelaRenderer STATIC
	${AccelaRenderer_Include_Headers}
	${AccelaRenderer_Include_Headers_Task}
	${AccelaRenderer_Include_Headers_Texture}
	${AccelaRenderer_Include_Headers_Graph}
	${AccelaRenderer_Include_Headers_Renderable}
	${AccelaRenderer_Include_Headers_Shader}
	${AccelaRenderer_Include_Headers_Util}
	${AccelaRenderer_Include_Headers_Mesh}
	${AccelaRenderer_Include_Headers_Material}

	${AccelaRenderer_Sources}
	${AccelaRenderer_Headers}
	${AccelaRenderer_Sources_Task}
	${AccelaRenderer_Headers_Task}
	${AccelaRenderer_Sources_Util}
	${AccelaRenderer_Headers_Util}
)

target_compile_features(AccelaRenderer
	PUBLIC
		cxx_std_23
)

if (NOT MSVC)
	target_compile_options(AccelaRenderer PRIVATE ${ACCELA_WARNINGS_FLAGS})
endif()

target_compile_definitions(AccelaRenderer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_link_libraries(AccelaRenderer
	PUBLIC
		AccelaCommon
		glm::glm
)

target_include_directories(AccelaRenderer
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