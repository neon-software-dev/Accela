cmake_minimum_required(VERSION 3.26.0)

project(AccelaPlatformDesktopSDL VERSION 0.0.1 LANGUAGES CXX)

	find_package(SDL2 CONFIG REQUIRED)

	file(GLOB AccelaPlatformDesktopSDL_Include_Headers "include/Accela/Platform/*.h")
	file(GLOB AccelaPlatformDesktopSDL_Include_Window_Headers "include/Accela/Platform/Window/*.h")

	file(GLOB AccelaPlatformDesktopSDL_Sources "src/*.cpp")
	file(GLOB AccelaPlatformDesktopSDL_Headers "src/*.h")
	file(GLOB AccelaPlatformDesktopSDL_Sources_Event "src/Event/*.cpp")
	file(GLOB AccelaPlatformDesktopSDL_Headers_Event "src/Event/*.h")
	file(GLOB AccelaPlatformDesktopSDL_Sources_Window "src/Window/*.cpp")
	file(GLOB AccelaPlatformDesktopSDL_Headers_Window "src/Window/*.h")

add_library(AccelaPlatformDesktopSDL
	${AccelaPlatformDesktopSDL_Include_Headers}
	${AccelaPlatformDesktopSDL_Include_Window_Headers}

	${AccelaPlatformDesktopSDL_Sources}
	${AccelaPlatformDesktopSDL_Headers}
	${AccelaPlatformDesktopSDL_Sources_Event}
	${AccelaPlatformDesktopSDL_Headers_Event}
	${AccelaPlatformDesktopSDL_Sources_Window}
	${AccelaPlatformDesktopSDL_Headers_Window}
)

target_link_libraries(AccelaPlatformDesktopSDL
	PUBLIC
		AccelaPlatformDesktop
		SDL2::SDL2
)

target_compile_features(AccelaPlatformDesktopSDL PUBLIC cxx_std_23 PRIVATE cxx_std_23)

target_compile_options(AccelaPlatformDesktopSDL PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_include_directories(AccelaPlatformDesktopSDL
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
