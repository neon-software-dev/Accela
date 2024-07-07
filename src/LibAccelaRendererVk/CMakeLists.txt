cmake_minimum_required(VERSION 3.19.0)

project(LibAccelaRendererVk VERSION 0.0.1 LANGUAGES CXX)

	find_package(Vulkan REQUIRED)
	find_package(VulkanMemoryAllocator CONFIG REQUIRED)
	find_package(unofficial-spirv-reflect CONFIG REQUIRED)

	file(GLOB AccelaRendererVk_Include_Headers "include/Accela/Render/*.h")

	file(GLOB AccelaRendererVk_Sources "src/*.cpp")
	file(GLOB AccelaRendererVk_Headers "src/*.h")
	file(GLOB AccelaRendererVk_Sources_VMA "src/VMA/*.cpp")
	file(GLOB AccelaRendererVk_Headers_VMA "src/VMA/*.h")
	file(GLOB AccelaRendererVk_Sources_Vulkan "src/Vulkan/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Vulkan "src/Vulkan/*.h")
	file(GLOB AccelaRendererVk_Sources_Shader "src/Shader/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Shader "src/Shader/*.h")
	file(GLOB AccelaRendererVk_Sources_Program "src/Program/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Program "src/Program/*.h")
	file(GLOB AccelaRendererVk_Sources_Util "src/Util/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Util "src/Util/*.h")
	file(GLOB AccelaRendererVk_Sources_Pipeline "src/Pipeline/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Pipeline "src/Pipeline/*.h")
	file(GLOB AccelaRendererVk_Sources_Texture "src/Texture/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Texture "src/Texture/*.h")
	file(GLOB AccelaRendererVk_Sources_Buffer "src/Buffer/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Buffer "src/Buffer/*.h")
	file(GLOB AccelaRendererVk_Sources_Renderer "src/Renderer/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Renderer "src/Renderer/*.h")
	file(GLOB AccelaRendererVk_Sources_Mesh "src/Mesh/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Mesh "src/Mesh/*.h")
	file(GLOB AccelaRendererVk_Sources_Framebuffer "src/Framebuffer/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Framebuffer "src/Framebuffer/*.h")
	file(GLOB AccelaRendererVk_Sources_Renderables "src/Renderables/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Renderables "src/Renderables/*.h")
	file(GLOB AccelaRendererVk_Sources_Material "src/Material/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Material "src/Material/*.h")
	file(GLOB AccelaRendererVk_Sources_Light "src/Light/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Light "src/Light/*.h")
	file(GLOB AccelaRendererVk_Sources_RenderTarget "src/RenderTarget/*.cpp")
	file(GLOB AccelaRendererVk_Headers_RenderTarget "src/RenderTarget/*.h")
	file(GLOB AccelaRendererVk_Sources_Image "src/Image/*.cpp")
	file(GLOB AccelaRendererVk_Headers_Image "src/Image/*.h")

add_library(AccelaRendererVk STATIC
	${AccelaRendererVk_Include_Headers}

	${AccelaRendererVk_Sources}
	${AccelaRendererVk_Headers}
	${AccelaRendererVk_Sources_VMA}
	${AccelaRendererVk_Headers_VMA}
	${AccelaRendererVk_Sources_Vulkan}
	${AccelaRendererVk_Headers_Vulkan}
	${AccelaRendererVk_Sources_Shader}
	${AccelaRendererVk_Headers_Shader}
	${AccelaRendererVk_Sources_Program}
	${AccelaRendererVk_Headers_Program}
	${AccelaRendererVk_Sources_Util}
	${AccelaRendererVk_Headers_Util}
	${AccelaRendererVk_Sources_Pipeline}
	${AccelaRendererVk_Headers_Pipeline}
	${AccelaRendererVk_Sources_Texture}
	${AccelaRendererVk_Headers_Texture}
	${AccelaRendererVk_Sources_Buffer}
	${AccelaRendererVk_Headers_Buffer}
	${AccelaRendererVk_Sources_Renderer}
	${AccelaRendererVk_Headers_Renderer}
	${AccelaRendererVk_Sources_Mesh}
	${AccelaRendererVk_Headers_Mesh}
	${AccelaRendererVk_Sources_Framebuffer}
	${AccelaRendererVk_Headers_Framebuffer}
	${AccelaRendererVk_Sources_Renderables}
	${AccelaRendererVk_Headers_Renderables}
	${AccelaRendererVk_Sources_Material}
	${AccelaRendererVk_Headers_Material}
	${AccelaRendererVk_Sources_Light}
	${AccelaRendererVk_Headers_Light}
	${AccelaRendererVk_Sources_RenderTarget}
	${AccelaRendererVk_Headers_RenderTarget}
	${AccelaRendererVk_Sources_Image}
	${AccelaRendererVk_Headers_Image}
)

target_compile_features(AccelaRendererVk
	PUBLIC
		cxx_std_23
)

target_compile_options(AccelaRendererVk PRIVATE ${ACCELA_WARNINGS_FLAGS})

target_link_libraries(AccelaRendererVk
	PUBLIC
		AccelaCommon
		AccelaRenderer
	PRIVATE
		Vulkan::Vulkan
		GPUOpen::VulkanMemoryAllocator
		unofficial::spirv-reflect::spirv-reflect
)

target_include_directories(AccelaRendererVk
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
