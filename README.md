# TheWorld_GDN_Viewer

## Build the solution with CMake (Windows, Visual Studio, x64)

CMake builds the projects in `TheWorld_GDN_Viewer.sln` without changing the solution or Visual Studio project files. The CMake project files for `TheWorld_MapManager`, `TheWorld_Utils`, `TheWorld_Utils_UnitTest`, and `lib_json` are in their existing sibling project directories.

From this repository root, configure and build with:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug
```

Use `--config Release` for a release build. Run the native unit tests with:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Visual Studio's Open Folder workflow also supports Ninja builds. Use `CMAKE_BUILD_TYPE=Debug` or `Release` for single-configuration generators. The unit-test target supplies the library search directory required by MSTest's header autolinking.

All solution DLLs and libraries are generated directly in `x64` (or the directory set by `THEWORLD_LIB_DIR`), without Debug or Release subdirectories. Debug files have the `_d` suffix, for example `x64\TheWorld_GDN_Viewer_d.dll`; Release files do not, for example `x64\TheWorld_GDN_Viewer.dll`. The two configurations can coexist. Build intermediates remain separate in the CMake build directory. Regenerate CMake after changing the output layout; old build outputs are not moved or deleted automatically.

After linking the viewer, CMake copies its DLL and the matching MapManager and Utils DLLs to `..\TheWorld_Viewer\TheWorld\addons\twviewer\native`. The destination must already exist. Set `THEWORLD_NATIVE_DIR` to use another directory, or set `THEWORLD_DEPLOY_NATIVE=OFF` to disable copying. Unlike the Visual Studio wildcard copy, this copies only these three DLLs for the configuration being built, not unrelated or stale DLLs. As a post-build step, it runs when the viewer is built or relinked; a no-op build does not redeploy deleted files.

The build expects the existing sibling dependency directories under the parent directory of this checkout, including `godot-cpp`, `GSL`, Eigen, `PROJ`, `TheWorld_MapManager`, `TheWorld_Utils`, `TheWorld_Utils_UnitTest`, and `jsoncpp`. The Visual Studio Native Unit Test framework and `vstest.console.exe` are needed to build and run the unit-test project. PROJ and godot-cpp are linked from their existing prebuilt libraries. Override `THEWORLD_EXTERNAL_ROOT`, `THEWORLD_LIB_DIR`, `THEWORLD_PROJ_LIB_DIR`, `THEWORLD_GODOT_CPP_DIR`, or `THEWORLD_GODOT_CPP_LIB_DIR` at configure time if your directory layout differs.
