cmake_minimum_required(VERSION 3.30.5)
project(DmxRasterizer VERSION 0.7.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CCX=clang++)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

set(APP_ICO "${CMAKE_SOURCE_DIR}/src/resource/icon.rc")

include(FetchContent)

option(WERELIB_LOCAL "Use local Werelib" OFF)
if(WERELIB_LOCAL)
	if(NOT DEFINED WERELIB_SRC)
		message(FATAL_ERROR "Please define WERELIB_SRC (`-DWERELIB_SRC=/path/to/Werelib)")
	endif()
	message(STATUS "Using local Werelib.")
	FetchContent_Declare(
		Werelib
		SOURCE_DIR ${WERELIB_SRC}
	)
	FetchContent_MakeAvailable(Werelib)
endif()

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

add_library(glad STATIC src/external/vendor/glad.c)
target_include_directories(glad PUBLIC src/external/vendor)

file(GLOB SOURCE_FILES src/*.cpp) # Grab all .cpp files from src folder
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${APP_ICO})

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
