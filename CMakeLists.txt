cmake_minimum_required(VERSION 3.31)
project(DmxRasterizer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

if(DEFINED ENV{EXTERNAL_CXX_LIBRARIES})
	file(TO_CMAKE_PATH "$ENV{EXTERNAL_CXX_LIBRARIES}" EXTRN)
else()
	message(FATAL_ERROR "Environment variable EXTRN is not set!")
endif()

include_directories(${EXTRN}/GLAD/include)
include_directories(${EXTRN}/GLFW/include)
include_directories(${EXTRN}/GLM)
include_directories(${EXTRN}/SPOUT/include)

add_library(glad STATIC "${EXTRN}/glad/src/glad.c")

file(GLOB SOURCE_FILES src/*.cpp) # Grab all .cpp files from src folder

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${PROJECT_NAME} PRIVATE
	glad
	opengl32.lib
	gdi32.lib
	"${EXTRN}/GLFW/lib/glfw3.lib"
	"${EXTRN}/SPOUT/MD/lib/Spout_static.lib"
	"${EXTRN}/SPOUT/MD/lib/SpoutLibrary.lib"
)