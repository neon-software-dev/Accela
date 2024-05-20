cmake_minimum_required(VERSION 3.19.0)

project(AccelaEditor VERSION 0.0.1)

	find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)
	qt_standard_project_setup()

	file(GLOB AccelaEditor_Sources "*.cpp")
	file(GLOB AccelaEditor_Headers "*.h")
	file(GLOB AccelaEditor_Sources_Window "Window/*.cpp")
	file(GLOB AccelaEditor_Headers_Window "Window/*.h")
	file(GLOB AccelaEditor_Sources_EditorScene "EditorScene/*.cpp")
	file(GLOB AccelaEditor_Headers_EditorScene "EditorScene/*.h")
	file(GLOB AccelaEditor_Sources_Thread "Thread/*.cpp")
	file(GLOB AccelaEditor_Headers_Thread "Thread/*.h")

qt_add_executable(AccelaEditor
	${AccelaEditor_Sources}
	${AccelaEditor_Headers}
	${AccelaEditor_Sources_Window}
	${AccelaEditor_Headers_Window}
	${AccelaEditor_Sources_EditorScene}
	${AccelaEditor_Headers_EditorScene}
	${AccelaEditor_Sources_Thread}
	${AccelaEditor_Headers_Thread}
)

target_compile_features(AccelaEditor PUBLIC cxx_std_23)

target_compile_options(AccelaEditor PRIVATE ${ACCELA_WARNINGS_FLAGS})

set_target_properties(AccelaEditor
	PROPERTIES
		WIN32_EXECUTABLE ON
		MACOSX_BUNDLE ON
)

target_link_libraries(AccelaEditor
	PRIVATE
		AccelaEngineDesktop
		AccelaRendererVk
		AccelaPlatformDesktopQt
		Qt6::Widgets
		Qt6::Gui
)
