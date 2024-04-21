cmake_minimum_required(VERSION 3.19.0)

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
# Set up custom commands to copy everything from the assets directory to the build output's assets directory
#
file (GLOB_RECURSE resources "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*")

foreach(resource ${resources})
	get_filename_component(filename ${resource} NAME) # file.png
	get_filename_component(dir ${resource} DIRECTORY) # /path/to/assets/textures/
	get_filename_component(dirname ${dir} NAME) # textures/

	set (asset_local_dir "")

	while(NOT ${dirname} STREQUAL "assets")
		get_filename_component(path_component ${dir} NAME)
		set (asset_local_dir "${path_component}/${asset_local_dir}")
		get_filename_component(dir ${dir} DIRECTORY)
		get_filename_component(dirname ${dir} NAME)
	endwhile()

	set(asset_local_path "${asset_local_dir}${filename}") # textures/file.png
	set(install_file "${CMAKE_CURRENT_BINARY_DIR}/assets/${asset_local_path}") # /path/to/bin/assets/textures/file.png

	add_custom_command(
			COMMENT "Asset file install '${asset_local_path}'"
			OUTPUT ${install_file}
			DEPENDS ${resource}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${resource}
			${install_file}
	)
	add_custom_target("${dirname}-${filename}-copy" ALL DEPENDS ${resource} ${install_file})

endforeach()
#
##########
