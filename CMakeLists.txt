cmake_minimum_required(VERSION 4.2.0)
project(DmxRasterizer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(APP_ICO "${CMAKE_SOURCE_DIR}/resource/icon.rc")
add_compile_options(-Wno-c23-extensions)

include(FetchContent)
FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	spout2
	URL https://github.com/leadedge/Spout2/archive/refs/tags/2.007.015.zip
	# Direct link because FetchContent will download the entire repo history (GIT_SHALLOW is limited)
	# DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	# GIT_REPOSITORY https://github.com/leadedge/Spout2.git
	# GIT_TAG fc63ac2c16918ffa1d3384cddfba2bbc70650bb4 # 2.007.015
	# GIT_SHALLOW TRUE
	# GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(spout2)

set(SOURCE_FILES 
	src/main.cpp
)
set_source_files_properties(${SOURCE_FILES} PROPERTIES
	COMPILE_OPTIONS "-fno-exceptions"
)

set(MODULE_FILES
	src/external/werelib/weretype.cppm
	src/external/werelib/fmt.cppm

	src/shader.cppm
	src/appState.cppm
	src/netThread.cppm

	src/mod/net/artnet.cppm
	src/mod/net/winsock.cppm
	
	src/mod/render/render.cppm
	src/mod/render/ui.cppm
	
	src/mod/applog.cppm
)
set_source_files_properties(${MODULE_FILES} PROPERTIES
	COMPILE_OPTIONS "-fno-exceptions"
)

set(IMGUI_SRC
	src/external/vendor/imgui/imgui.cpp
	src/external/vendor/imgui/imgui_draw.cpp
	src/external/vendor/imgui/imgui_tables.cpp
	src/external/vendor/imgui/imgui_widgets.cpp
	src/external/vendor/imgui/backends/imgui_impl_glfw.cpp
	src/external/vendor/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC src/external/vendor/imGui src/external/vendor/imGui/backends)
target_link_libraries(imgui PUBLIC glfw)

add_library(glad STATIC src/external/vendor/glad.c)
target_include_directories(glad PUBLIC src/external/vendor)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${APP_ICO})

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
	imgui
	Spout_static
)
