cmake_minimum_required(VERSION 3.26.0)

project(TestDesktopApp VERSION 0.0.1)

	file(GLOB TestDesktopApp_Sources "*.cpp")
	file(GLOB TestDesktopApp_Headers "*.h")
	
add_executable(TestDesktopApp 
	${TestDesktopApp_Sources}
	${TestDesktopApp_Headers}
)

target_compile_features(TestDesktopApp PUBLIC cxx_std_23)

target_link_libraries(TestDesktopApp
	PRIVATE
		AccelaEngineDesktop
)

##########
# Set up custom commands to copy accela directory to the build output directory
#
file (GLOB_RECURSE ACCELA_INPUT_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/accela/*.*")
file (GLOB_RECURSE ACCELA_OUTPUT_RESOURCES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/accela/*.*")

add_custom_target("CopyAccelaResources" ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/accela/" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/accela/"
		DEPENDS ${ACCELA_INPUT_RESOURCES} ${ACCELA_OUTPUT_RESOURCES}
		COMMENT "Copying Accela Resources"
)
#
##########
