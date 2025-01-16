## Build Requirements
**TOOLS**
- Cmake	(`winget install Kitware.CMake`)
- Ninja (`winget install Ninja-build.Ninja`)
- Clang (`winget install LLVM.LLVM`)
- MSVC 	(`winget install Microsoft.VisualStudio.2022.BuildTools`)

**ENVIRONMENT**
- Envrionment Variable: `EXTERNAL_CXX_LIBRARIES` needs to be created and pointed at your root folder that contains GLM / GLFW folders.

## Build commands (From project root directory)
- `cmake -G Ninja -B ./build`
- `ninja -C build`
- `./build/DmxRasterizer.exe`

# Quick tip
- On Windows 11 you can quickly get to editing your envrionment variables UI by writing `rundll32.exe sysdm.cpl,EditEnvironmentVariables` in your run prompt