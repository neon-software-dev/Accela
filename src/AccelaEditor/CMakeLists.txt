cmake_minimum_required(VERSION 3.26.0)

project(AccelaEditor VERSION 0.0.1)

	find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)
	qt_standard_project_setup()

	file(GLOB AccelaEditor_Sources "*.cpp")
	file(GLOB AccelaEditor_Headers "*.h")
	file(GLOB AccelaEditor_Sources_View "View/*.cpp")
	file(GLOB AccelaEditor_Headers_View "View/*.h")
	file(GLOB AccelaEditor_Sources_EditorScene "EditorScene/*.cpp")
	file(GLOB AccelaEditor_Headers_EditorScene "EditorScene/*.h")
	file(GLOB AccelaEditor_Sources_Thread "Thread/*.cpp")
	file(GLOB AccelaEditor_Headers_Thread "Thread/*.h")
	file(GLOB AccelaEditor_Sources_ViewModel "ViewModel/*.cpp")
	file(GLOB AccelaEditor_Headers_ViewModel "ViewModel/*.h")
	file(GLOB AccelaEditor_Sources_Util "Util/*.cpp")
	file(GLOB AccelaEditor_Headers_Util "Util/*.h")

	qt_add_resources(AccelaEditor_Sources "Resources/resources.qrc")

qt_add_executable(AccelaEditor
	${AccelaEditor_Sources}
	${AccelaEditor_Headers}
	${AccelaEditor_Sources_View}
	${AccelaEditor_Headers_View}
	${AccelaEditor_Sources_EditorScene}
	${AccelaEditor_Headers_EditorScene}
	${AccelaEditor_Sources_Thread}
	${AccelaEditor_Headers_Thread}
	${AccelaEditor_Sources_ViewModel}
	${AccelaEditor_Headers_ViewModel}
	${AccelaEditor_Sources_Util}
	${AccelaEditor_Headers_Util}
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
