cmake_minimum_required(VERSION 3.26.0)

project(AccelaPlatform VERSION 0.0.1 LANGUAGES CXX)

	find_package(Vulkan REQUIRED)

	file(GLOB AccelaPlatform_Include_Headers "include/Accela/Platform/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Event "include/Accela/Platform/Event/*.h")
	file(GLOB AccelaPlatform_Include_Headers_File "include/Accela/Platform/File/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Text "include/Accela/Platform/Text/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Window "include/Accela/Platform/Window/*.h")
	file(GLOB AccelaPlatform_Include_Headers_VR "include/Accela/Platform/VR/*.h")
	file(GLOB AccelaPlatform_Include_Headers_Package "include/Accela/Platform/Package/*.h")

add_library(AccelaPlatform
	INTERFACE
		${AccelaPlatform_Include_Headers}
		${AccelaPlatform_Include_Headers_Event}
		${AccelaPlatform_Include_Headers_File}
		${AccelaPlatform_Include_Headers_Text}
		${AccelaPlatform_Include_Headers_Window}
		${AccelaPlatform_Include_Headers_VR}
		${AccelaPlatform_Include_Headers_Package}
)

target_link_libraries(AccelaPlatform
	INTERFACE
		AccelaCommon
		AccelaRenderer
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

install(DIRECTORY include/ DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_INCLUDEDIR})
