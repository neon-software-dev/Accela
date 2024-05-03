cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaEngineDesktop VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB AccelaEngineDesktop_Include_Headers "include/Accela/Engine/*.h")

	file(GLOB AccelaEngineDesktop_Sources "src/*.cpp")
	file(GLOB AccelaEngineDesktop_Headers "src/*.h")

add_library(AccelaEngineDesktop STATIC
	${AccelaEngineDesktop_Include_Headers}

	${AccelaEngineDesktop_Sources}
	${AccelaEngineDesktop_Headers}
)

target_compile_features(AccelaEngineDesktop
	PUBLIC
		cxx_std_23
)

if (NOT MSVC)
	target_compile_options(AccelaEngineDesktop PRIVATE ${ACCELA_WARNINGS_FLAGS})
endif()

target_link_libraries(AccelaEngineDesktop
	PUBLIC
		AccelaEngine
		AccelaCommon
	PRIVATE
		AccelaPlatformDesktop
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
