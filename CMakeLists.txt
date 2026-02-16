cmake_minimum_required(VERSION 4.1.0)
project(DmxRasterizer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(APP_ICO "${CMAKE_SOURCE_DIR}/src/embed/resource/icon.rc")
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
)
FetchContent_MakeAvailable(spout2)

set(SOURCE_FILES 
	src/main.cpp
)
set_source_files_properties(${SOURCE_FILES} PROPERTIES
	COMPILE_OPTIONS "-fno-exceptions"
)

set(MODULE_FILES
	src/vendor/werelib/weretype.cppm
	src/vendor/werelib/fmt.cppm

	src/appState.cppm
	src/netThread.cppm
	
	src/mod/net/artnet.cppm
	src/mod/net/winsock.cppm
	
	src/mod/render/shader.cppm
	src/mod/render/render.cppm
	src/mod/render/ui.cppm
	
	src/mod/applog.cppm
)
set_source_files_properties(${MODULE_FILES} PROPERTIES
	COMPILE_OPTIONS "-fno-exceptions"
)

set(IMGUI_SRC
	src/vendor/imgui/imgui.cpp
	src/vendor/imgui/imgui_draw.cpp
	src/vendor/imgui/imgui_tables.cpp
	src/vendor/imgui/imgui_widgets.cpp
	src/vendor/imgui/backends/imgui_impl_glfw.cpp
	src/vendor/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC src/vendor/imGui src/vendor/imGui/backends)
target_link_libraries(imgui PUBLIC glfw)

add_library(glad STATIC src/vendor/glad/glad.c)
target_include_directories(glad PUBLIC src/vendor/glad/)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${APP_ICO})
target_include_directories(${PROJECT_NAME} PRIVATE src/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	EMBED_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/src/embed
)

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
	ws2_32.lib
	glad
	glfw
	imgui
	Spout_static
)

# Hide console only in Release
set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE $<CONFIG:Release>
)

# In Release, use main() as entrypoint even under GUI subsystem
target_link_options(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Release>:-Wl,/ENTRY:mainCRTStartup>
)