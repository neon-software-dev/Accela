cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaEngineDesktop VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB AccelaEngineDesktop_Include_Headers "include/Accela/Engine/*.h")
	file(GLOB AccelaEngineDesktop_Include_Headers_Package "include/Accela/Engine/Package/*.h")

	file(GLOB AccelaEngineDesktop_Sources "src/*.cpp")
	file(GLOB AccelaEngineDesktop_Headers "src/*.h")
	file(GLOB AccelaEngineDesktop_Sources_Package "src/Package/*.cpp")
	file(GLOB AccelaEngineDesktop_Headers_Package "src/Package/*.h")

add_library(AccelaEngineDesktop STATIC
	${AccelaEngineDesktop_Include_Headers}
	${AccelaEngineDesktop_Include_Headers_Package}

	${AccelaEngineDesktop_Sources}
	${AccelaEngineDesktop_Headers}
	${AccelaEngineDesktop_Sources_Package}
	${AccelaEngineDesktop_Headers_Package}
)

target_compile_features(AccelaEngineDesktop
	PUBLIC
		cxx_std_23
)

target_compile_options(AccelaEngineDesktop PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_link_libraries(AccelaEngineDesktop
	PUBLIC
		AccelaEngine
		AccelaCommon
	PRIVATE
		AccelaPlatformDesktopSDL
		AccelaRendererVk
)

target_include_directories(AccelaEngineDesktop
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
