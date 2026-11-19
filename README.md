# TheWorld_GDN_Viewer

## jsoncpp in the Visual Studio solution

`..\build\TerrainTechDemo.sln` includes `jsoncpp` through `..\cmake\jsoncpp\jsoncpp.vcxproj`, a Makefile project that configures and builds the existing `..\jsoncpp\CMakeLists.txt`. It requires Visual Studio 2026, the v145 toolset, and the C++ CMake tools component (or `cmake` on PATH). Build, Rebuild, and Clean invoke CMake automatically; no generated project files need to be added manually.

The adapter builds the static library with the DLL MSVC runtime, producing `..\build\x64\lib_json_d.lib` for Debug and `..\build\x64\lib_json.lib` for Release, preserving the existing linker inputs. Win32 builds use `..\build\Win32`. Generated jsoncpp projects and intermediates are kept under `..\build\out\jsoncpp-msbuild\<Platform>` so the Visual Studio build does not reuse caches tied to the adapter's former location. MapManager and the viewer depend on this project, so jsoncpp is built before linking them. The upstream jsoncpp sources and CMake files remain unchanged.

## PROJ in the Visual Studio solution

The `proj` project builds the existing `PROJ\CMakeLists.txt` through `..\cmake\proj`. MapManager depends on it and links the static `..\build\x64\proj_d.lib` or `proj.lib`. It uses the SQLite amalgamation already in `TheWorld_MapManager`; the generated `proj.db` and standard PROJ resources are embedded, so no PROJ DLL or external database is needed at runtime. As in the previous PROJ build, TIFF grids and network access are disabled.

To generate `proj.db`, PROJ looks for `TerrainTechDemo\DipendenzeEsterne\sqlite3.exe` before searching PATH. To use a different executable, set `ProjSqlite3Executable` in MSBuild or `EXE_SQLITE3` in CMake. If an existing CMake cache still points to a different copy, reconfigure with `-U EXE_SQLITE3` to select the local copy. For example, from `TerrainTechDemo`, to refresh the Visual Studio project's cache:

```powershell
cmake -S .\cmake\proj -B .\build\out\proj\x64 -U EXE_SQLITE3
```

CMake 3.22.1 or newer is required. The adapter targets x64; its generated files are under `build\out\proj\x64`.

## Build the solution with CMake (Windows, Visual Studio, x64)

The solution and top-level `CMakeLists.txt` are in `TerrainTechDemo\build`, one directory above this checkout. Their project and source references are relative. The parent repository tracks these two entry points while ignoring generated files in `build`.

CMake builds the solution projects independently of the Visual Studio project files. The CMake project files for `TheWorld_MapManager`, `TheWorld_Utils`, and `TheWorld_Utils_UnitTest` are in their existing sibling project directories. Both build workflows use `..\cmake\jsoncpp\CMakeLists.txt` to adapt the upstream jsoncpp target to the existing `lib_json` name and output layout.

From this repository root, configure and build with:

```powershell
cmake -S ..\build -B ..\build\out\cmake -A x64
cmake --build ..\build\out\cmake --config Debug
```

Use a new binary directory: existing CMake caches still refer to the old source location and must not be reused. Do not configure directly into `..\build`, because generated solution files could overwrite the maintained solution. Open `..\build` for Visual Studio's CMake Open Folder workflow, or open `..\build\TerrainTechDemo.sln` for MSBuild.

Use `--config Release` for a release build. Run the native unit tests with:

```powershell
ctest --test-dir ..\build\out\cmake -C Debug --output-on-failure
```

Visual Studio's Open Folder workflow also supports Ninja builds. Use `CMAKE_BUILD_TYPE=Debug` or `Release` for single-configuration generators. The unit-test target supplies the library search directory required by MSTest's header autolinking.

CMake automatically migrates cached `THEWORLD_GODOT_CPP_DIR` and `THEWORLD_GODOT_CPP_LIB_DIR` values from the former default under `Client` when those directories no longer exist and the new directories are available. Other custom paths are preserved, and invalid paths still produce an error.

To reset other stale paths to the defaults, run from a Developer PowerShell:

```powershell
cmake -S ..\build -B ..\build\out\x64-debug -U THEWORLD_GODOT_CPP_DIR -U THEWORLD_GODOT_CPP_LIB_DIR
cmake -S ..\build -B ..\build\out\x64-release -U THEWORLD_GODOT_CPP_DIR -U THEWORLD_GODOT_CPP_LIB_DIR
```

Use the actual build directories if they differ (for example, `build`). Visual Studio's Debug and Release caches are independent. If you use custom paths, supply the updated values with `-D` instead.

All solution DLLs and libraries are generated directly in `..\build\x64` (or the directory set by `THEWORLD_LIB_DIR`), without Debug or Release subdirectories. Debug files have the `_d` suffix, for example `..\build\x64\TheWorld_GDN_Viewer_d.dll`; Release files do not, for example `..\build\x64\TheWorld_GDN_Viewer.dll`. The two configurations can coexist. Build intermediates remain separate in the CMake binary directory. Old build outputs are not moved or deleted automatically.

After linking the viewer, CMake copies its DLL and the matching MapManager and Utils DLLs to `..\TheWorld_Viewer\TheWorld\addons\twviewer\native`. The destination must already exist. Set `THEWORLD_NATIVE_DIR` to use another directory, or set `THEWORLD_DEPLOY_NATIVE=OFF` to disable copying. Unlike the Visual Studio wildcard copy, this copies only these three DLLs for the configuration being built, not unrelated or stale DLLs. As a post-build step, it runs when the viewer is built or relinked; a no-op build does not redeploy deleted files.

The build expects `GSL`, Eigen, `PROJ`, `TheWorld_MapManager`, `TheWorld_Utils`, `TheWorld_Utils_UnitTest`, and `jsoncpp` under the parent directory of this checkout. The `godot`, `godot-cpp`, and `extension_api` directories are siblings of `Client` (for example, `..\godot-cpp` relative to `Client`). The Visual Studio Native Unit Test framework and `vstest.console.exe` are needed to build and run the unit-test project. PROJ is built from source; godot-cpp is linked from its existing prebuilt libraries. Override `THEWORLD_EXTERNAL_ROOT`, `THEWORLD_LIB_DIR`, `THEWORLD_GODOT_CPP_DIR`, or `THEWORLD_GODOT_CPP_LIB_DIR` at configure time if your directory layout differs. Standalone MapManager builds can still use prebuilt PROJ via `THEWORLD_PROJ_LIB_DIR`.
