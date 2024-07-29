cmake_minimum_required(VERSION 3.26.0)

project(LibAccelaPlatformDesktopQt VERSION 0.0.1 LANGUAGES CXX)

	find_package(Qt6 REQUIRED COMPONENTS Widgets)

	file(GLOB AccelaPlatformDesktopQt_Include_Headers "include/Accela/Platform/*.h")
	file(GLOB AccelaPlatformDesktopQt_Include_Window_Headers "include/Accela/Platform/Window/*.h")
	file(GLOB AccelaPlatformDesktopQt_Include_Event_Headers "include/Accela/Platform/Event/*.h")

	file(GLOB AccelaPlatformDesktopQt_Sources "src/*.cpp")
	file(GLOB AccelaPlatformDesktopQt_Headers "src/*.h")
	file(GLOB AccelaPlatformDesktopQt_Sources_Event "src/Event/*.cpp")
	file(GLOB AccelaPlatformDesktopQt_Headers_Event "src/Event/*.h")
	file(GLOB AccelaPlatformDesktopQt_Sources_Window "src/Window/*.cpp")
	file(GLOB AccelaPlatformDesktopQt_Headers_Window "src/Window/*.h")

add_library(AccelaPlatformDesktopQt STATIC
	${AccelaPlatformDesktopQt_Include_Headers}
	${AccelaPlatformDesktopQt_Include_Window_Headers}
	${AccelaPlatformDesktopQt_Include_Event_Headers}

	${AccelaPlatformDesktopQt_Sources}
	${AccelaPlatformDesktopQt_Headers}
	${AccelaPlatformDesktopQt_Sources_Event}
	${AccelaPlatformDesktopQt_Headers_Event}
	${AccelaPlatformDesktopQt_Sources_Window}
	${AccelaPlatformDesktopQt_Headers_Window}
)

target_compile_features(AccelaPlatformDesktopQt
	PUBLIC
		cxx_std_23
)

target_compile_options(AccelaPlatformDesktopQt PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_link_libraries(AccelaPlatformDesktopQt
	PUBLIC
		AccelaPlatformDesktop
	PRIVATE
		Qt6::Widgets
)

target_include_directories(AccelaPlatformDesktopQt
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

# Install Qt dynamic libraries
install(IMPORTED_RUNTIME_ARTIFACTS Qt6::Widgets)
