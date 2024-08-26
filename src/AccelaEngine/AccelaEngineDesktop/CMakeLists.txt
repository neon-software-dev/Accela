cmake_minimum_required(VERSION 3.26.0)

project(AccelaEngineDesktop VERSION 0.0.1 LANGUAGES CXX)

	find_package(OpenXR CONFIG REQUIRED)

	file(GLOB AccelaEngineDesktop_Include_Headers "include/Accela/Engine/*.h")
	file(GLOB AccelaEngineDesktop_Include_Headers_Package "include/Accela/Engine/Package/*.h")

	file(GLOB AccelaEngineDesktop_Sources "src/*.cpp")
	file(GLOB AccelaEngineDesktop_Headers "src/*.h")
	file(GLOB AccelaEngineDesktop_Sources_Package "src/Package/*.cpp")
	file(GLOB AccelaEngineDesktop_Headers_Package "src/Package/*.h")


add_library(AccelaEngineDesktop
	${AccelaEngineDesktop_Include_Headers}
	${AccelaEngineDesktop_Include_Headers_Package}

	${AccelaEngineDesktop_Sources}
	${AccelaEngineDesktop_Headers}
	${AccelaEngineDesktop_Sources_Package}
	${AccelaEngineDesktop_Headers_Package}
)

target_link_libraries(AccelaEngineDesktop
	PUBLIC
		AccelaCommon
		AccelaEngine
	PRIVATE
		AccelaPlatformDesktopSDL
		AccelaRendererVk
		OpenXR::headers
		OpenXR::openxr_loader
)

target_compile_features(AccelaEngineDesktop PUBLIC cxx_std_23 PRIVATE cxx_std_23)

target_compile_options(AccelaEngineDesktop PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_compile_definitions(AccelaEngineDesktop PRIVATE XR_USE_GRAPHICS_API_VULKAN)

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

install(DIRECTORY include/ DESTINATION ${ACCELAENGINE_INSTALL_SUBDIR}${CMAKE_INSTALL_INCLUDEDIR})
