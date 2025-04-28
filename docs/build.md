## Build Requirements
**TOOLS**
- Cmake	(`winget install Kitware.CMake`)
- Ninja (`winget install Ninja-build.Ninja`)
- Clang (`winget install LLVM.LLVM`)
- MSVC 	(`winget install Microsoft.VisualStudio.2022.BuildTools`)

## Build commands (From project root directory)
- `cmake -G Ninja -B ./build`
- `ninja -C build`
- `./build/DmxRasterizer.exe`

**OPTIONAL BUILDS**
- Set Build to release mode
`cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Release`
- Set Build to specific compiler
`cmake -G Ninja -B ./build -DCMAKE_CXX_COMPILER=clang++`
- Use local Werelib library
`cmake -G Ninja -B build -DWERELIB_LOCAL=ON -DWERELIB_SRC="Path/To//wlfstn/werelib"`

## Build messages
- When building Spout2 for the first time it'll generate several warnings

# Quick tip
- On Windows 11 you can quickly get to editing your envrionment variables UI by writing `rundll32.exe sysdm.cpl,EditEnvironmentVariables` in your run prompt