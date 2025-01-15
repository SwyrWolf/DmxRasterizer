cmake_minimum_required(VERSION 3.10)
project(DmxRasterizer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Use the LIB environment variable
set(INC $ENV{INCLUDE})
set(GLAD_PATH_SRC "G:/PRJ/DEV/LIBS/GLAD/src/glad.c")

include_directories(${INC}/GLAD/include)
include_directories(${INC}/GLFW/include)
include_directories(${INC}/GLM)

set(SOURCE_FILES
   	main.cpp
		shader.cpp
		${GLAD_PATH_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
	${PROJECT_NAME}
	"G:/PRJ/DEV/LIBS/GLFW/lib/glfw3.lib"
)