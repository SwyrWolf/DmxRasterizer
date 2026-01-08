## Cmake example of local library / conditional fetch content
```cmake
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
```

## Example of Cmake Fetch Content
```cmake
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

set(MODULE_FILES
	src/external/werelib/weretype.cppm
	src/shader.cppm
	src/artnet.cppm
)
```