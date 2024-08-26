cmake_minimum_required(VERSION 3.26.0)

project(AccelaCommon VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB AccelaCommon_Include_Headers "include/Accela/Common/*.h")
	file(GLOB AccelaCommon_Include_Headers_Container "include/Accela/Common/Container/*.h")
	file(GLOB AccelaCommon_Include_Headers_Log "include/Accela/Common/Log/*.h")
	file(GLOB AccelaCommon_Include_Headers_Metrics "include/Accela/Common/Metrics/*.h")
	file(GLOB AccelaCommon_Include_Headers_Thread "include/Accela/Common/Thread/*.h")

	file(GLOB AccelaCommon_Sources "src/*.cpp")
	file(GLOB AccelaCommon_Headers "src/*.h")
	file(GLOB AccelaCommon_Sources_Log "src/Log/*.cpp")
	file(GLOB AccelaCommon_Headers_Log "src/Log/*.h")
	file(GLOB AccelaCommon_Sources_Metrics "src/Metrics/*.cpp")
	file(GLOB AccelaCommon_Headers_Metrics "src/Metrics/*.h")
	file(GLOB AccelaCommon_Sources_Thread "src/Thread/*.cpp")
	file(GLOB AccelaCommon_Headers_Thread "src/Thread/*.h")

add_library(AccelaCommon
	${AccelaCommon_Include_Headers}
	${AccelaCommon_Include_Headers_Container}
	${AccelaCommon_Include_Headers_Log}
	${AccelaCommon_Include_Headers_Metrics}
	${AccelaCommon_Include_Headers_Thread}

	${AccelaCommon_Sources}
	${AccelaCommon_Headers}
	${AccelaCommon_Sources_Log}
	${AccelaCommon_Headers_Log}
	${AccelaCommon_Sources_Metrics}
	${AccelaCommon_Headers_Metrics}
	${AccelaCommon_Sources_Thread}
	${AccelaCommon_Headers_Thread}
)

# TODO Release: Set proper soversioning on lib outputs
#set_target_properties(AccelaCommon PROPERTIES
#	SOVERSION 0
#	VERSION 0.0.1
#)

target_compile_features(AccelaCommon PUBLIC cxx_std_23 PRIVATE cxx_std_23)

target_compile_options(AccelaCommon PRIVATE ${ACCELA_WARNINGS_FLAGS})

if (ACCELA_TARGET_PLATFORM MATCHES ${ACCELA_TARGET_PLATFORM_DESKTOP})
	target_compile_definitions(AccelaCommon PUBLIC ACCELA_PLATFORM_DESKTOP)
elseif(ACCELA_TARGET_PLATFORM MATCHES ${ACCELA_TARGET_PLATFORM_ANDROID})
	target_compile_definitions(AccelaCommon PUBLIC ACCELA_PLATFORM_ANDROID)
endif()

target_include_directories(AccelaCommon
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
