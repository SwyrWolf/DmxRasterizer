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