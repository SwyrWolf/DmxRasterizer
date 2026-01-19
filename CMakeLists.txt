cmake_minimum_required(VERSION 4.2.0)
project(DmxRasterizer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(APP_ICO "${CMAKE_SOURCE_DIR}/resource/icon.rc")
add_compile_options(-Wno-c23-extensions)

include(FetchContent)
FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	spout2
	GIT_REPOSITORY https://github.com/leadedge/Spout2.git
	GIT_TAG 2.007.015
)
FetchContent_MakeAvailable(spout2)

set(SOURCE_FILES 
	src/main.cpp
)
set(MODULE_FILES
	src/external/werelib/weretype.cppm
	src/external/werelib/fmt.cppm

	src/shader.cppm
	src/artnet.cppm
	src/appState.cppm

	src/mod/net/artnet.cppm
	src/mod/net/winsock.cppm
	
	src/mod/render/render.cppm
	src/mod/render/ui.cppm
	
	src/mod/applog.cppm
)

set(IMGUI_SRC
	src/external/vendor/imgui/imgui.cpp
	src/external/vendor/imgui/imgui_draw.cpp
	src/external/vendor/imgui/imgui_tables.cpp
	src/external/vendor/imgui/imgui_widgets.cpp
	src/external/vendor/imgui/backends/imgui_impl_glfw.cpp
	src/external/vendor/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(glad STATIC src/external/vendor/glad.c)
target_include_directories(glad PUBLIC src/external/vendor)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${IMGUI_SRC} ${APP_ICO})

target_sources(${PROJECT_NAME} PRIVATE
	FILE_SET cxx_modules TYPE CXX_MODULES FILES
		${MODULE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${glfw_SOURCE_DIR}/include/GLFW
	${spout2_SOURCE_DIR}/SPOUTSDK
)

target_link_libraries(
	${PROJECT_NAME} PRIVATE
	opengl32.lib
	gdi32.lib
	glad
	glfw
	Spout_static
)
