cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaPlatform VERSION 0.0.1 LANGUAGES CXX)

	find_package(assimp CONFIG REQUIRED)

	file(GLOB AccelaPlatform_Include_Headers "include/Accela/Platform/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Events "include/Accela/Platform/Event/*.h")
	file(GLOB AccelaPlatform_Include_Headers_File "include/Accela/Platform/File/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Text "include/Accela/Platform/Text/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Window "include/Accela/Platform/Window/*.h")
	file(GLOB AccelaPlatform_Include_Headers_VR "include/Accela/Platform/VR/*.h")

add_library(AccelaPlatform INTERFACE
	${AccelaPlatform_Include_Headers}
	${AccelaPlatform_Include_Headers_Events}
	${AccelaPlatform_Include_Headers_File}
	${AccelaPlatform_Include_Headers_Text}
	${AccelaPlatform_Include_Headers_Window}
	${AccelaPlatform_Include_Headers_VR}
)

target_link_libraries(AccelaPlatform
	INTERFACE
		AccelaCommon
		assimp::assimp
)

target_include_directories(AccelaPlatform
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
