cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaPlatformDesktop VERSION 0.0.1 LANGUAGES CXX)

	find_package(SDL2 CONFIG REQUIRED)
	find_package(SDL2_image CONFIG REQUIRED)
	find_package(SDL2_ttf CONFIG REQUIRED)
	find_package(OpenVR REQUIRED)
	find_package(nlohmann_json CONFIG REQUIRED)

	file(GLOB AccelaPlatformDesktop_Include_Headers "include/Accela/Platform/*.h")
	file(GLOB AccelaPlatformDesktop_Include_File_Headers "include/Accela/Platform/File/*.h")
	file(GLOB AccelaPlatformDesktop_Include_Text_Headers "include/Accela/Platform/Text/*.h")
	file(GLOB AccelaPlatformDesktop_Include_VR_Headers "include/Accela/Platform/VR/*.h")
	file(GLOB AccelaPlatformDesktop_Include_Package_Headers "include/Accela/Platform/Package/*.h")

	file(GLOB AccelaPlatformDesktop_Sources "src/*.cpp")
	file(GLOB AccelaPlatformDesktop_Headers "src/*.h")
	file(GLOB AccelaPlatformDesktop_Sources_File "src/File/*.cpp")
	file(GLOB AccelaPlatformDesktop_Headers_File "src/File/*.h")
	file(GLOB AccelaPlatformDesktop_Sources_Text "src/Text/*.cpp")
	file(GLOB AccelaPlatformDesktop_Headers_Text "src/Text/*.h")
	file(GLOB AccelaPlatformDesktop_Sources_VR "src/VR/*.cpp")
	file(GLOB AccelaPlatformDesktop_Headers_VR "src/VR/*.h")
	file(GLOB AccelaPlatformDesktop_Sources_Package "src/Package/*.cpp")
	file(GLOB AccelaPlatformDesktop_Headers_Package "src/Package/*.h")

add_library(AccelaPlatformDesktop STATIC
	${AccelaPlatformDesktop_Include_Headers}
	${AccelaPlatformDesktop_Include_File_Headers}
	${AccelaPlatformDesktop_Include_Text_Headers}
	${AccelaPlatformDesktop_Include_VR_Headers}
	${AccelaPlatformDesktop_Include_Package_Headers}

	${AccelaPlatformDesktop_Sources}
	${AccelaPlatformDesktop_Headers}
	${AccelaPlatformDesktop_Sources_File}
	${AccelaPlatformDesktop_Headers_File}
	${AccelaPlatformDesktop_Sources_Text}
	${AccelaPlatformDesktop_Headers_Text}
	${AccelaPlatformDesktop_Sources_VR}
	${AccelaPlatformDesktop_Headers_VR}
	${AccelaPlatformDesktop_Sources_Package}
	${AccelaPlatformDesktop_Headers_Package}
)

target_compile_features(AccelaPlatformDesktop
	PUBLIC
		cxx_std_23
)

target_compile_options(AccelaPlatformDesktop PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_link_libraries(AccelaPlatformDesktop
	PUBLIC
		AccelaCommon
		AccelaPlatform
		SDL2::SDL2
	PRIVATE
		$<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
		$<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
		OpenVR::OpenVR
		nlohmann_json::nlohmann_json
)

target_include_directories(AccelaPlatformDesktop
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
