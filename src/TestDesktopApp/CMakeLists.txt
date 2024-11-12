cmake_minimum_required(VERSION 3.26.0)

project(TestDesktopApp VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB TestDesktopApp_Sources "*.cpp")
	file(GLOB TestDesktopApp_Headers "*.h")
	
add_executable(TestDesktopApp
	${TestDesktopApp_Sources}
	${TestDesktopApp_Headers}
)

target_compile_features(TestDesktopApp PRIVATE cxx_std_23)

target_link_libraries(TestDesktopApp
	PRIVATE
		AccelaEngineDesktop
)

install(
	TARGETS
		TestDesktopApp
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# If Windows, copy externally built binaries to the runtime output directory so TestDesktopApp can run.
# This is not needed on Linux due to Linux having rpath capability.
if (WIN32)
	add_custom_command(TARGET TestDesktopApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
			$<IF:$<CONFIG:Debug>,"${VCPKG_MANUAL_INSTALLED_DIR}/bin/","${VCPKG_MANUAL_INSTALLED_DIR}/debug/bin/">
			$<IF:$<CXX_COMPILER_ID:MSVC>,"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>","${CMAKE_RUNTIME_OUTPUT_DIRECTORY}">
		COMMENT "Copying external binaries"
	)
endif()

# Copy accela directory to the runtime output directory
#file (GLOB_RECURSE ACCELA_INPUT_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/accela/*.*")

#add_custom_command(TARGET TestDesktopApp POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#		"${CMAKE_CURRENT_SOURCE_DIR}/accela/"
#		$<IF:$<CXX_COMPILER_ID:MSVC>,"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/accela/","${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/accela/">
#	#DEPENDS ${ACCELA_INPUT_RESOURCES}
#	COMMENT "Copying accela directory"
#)

add_custom_target(CopyAccelaDir ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/accela/"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/accela/"
	COMMENT "Copying accela directory to runtime output directory"
)
add_dependencies(CopyAccelaDir TestDesktopApp)
