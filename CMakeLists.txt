cmake_minimum_required(VERSION 3.30.5)
project(DmxRasterizer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CCX=clang++)

set(APP_ICO "${CMAKE_SOURCE_DIR}/src/resource/icon.rc")

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

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${APP_ICO})

target_link_libraries(
	${PROJECT_NAME} PRIVATE
	glad
	opengl32.lib
	gdi32.lib
	"${EXTRN}/GLFW/lib/glfw3.lib"
	"${EXTRN}/SPOUT/MD/lib/Spout_static.lib"
	"${EXTRN}/SPOUT/MD/lib/SpoutLibrary.lib"
)