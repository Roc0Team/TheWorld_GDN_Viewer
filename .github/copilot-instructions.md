# TheWorld GDN Viewer contributor guide

## Build and test

- This is a Windows C++20 Godot 4 GDExtension DLL built by the Visual Studio solution with MSVC toolset `v145`. From a Developer PowerShell:

  ```powershell
  msbuild .\TheWorld_GDN_Viewer.sln /m /p:Configuration=Debug /p:Platform=x64
  ```

  Use `Configuration=Release` for the release extension. The solution also builds sibling projects outside this checkout: `..\TheWorld_MapManager`, `..\TheWorld_Utils`, and `..\jsoncpp`. The extension project additionally expects sibling `godot-cpp`, `GSL`, and Eigen directories; do not treat a missing dependency as an in-repo build failure.

- The extension has no in-repository application test project. `Quadtree` is a vendored, standalone CMake dependency with GoogleTest tests. Configure and run all of its tests with:

  ```powershell
  cmake -S .\Quadtree -B .\Quadtree\build -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=ON
  cmake --build .\Quadtree\build --target tests
  ctest --test-dir .\Quadtree\build --output-on-failure
  ```

  Run one parameterized test directly, for example:

  ```powershell
  .\Quadtree\build\tests\tests.exe --gtest_filter="SmallValues/QuadtreeTest.AddAndQueryTest/0"
  ```

## Architecture

- `TheWorld_GDN_Viewer\` is the extension implementation. `register_types.cpp` is the Godot 4 entry point: `tw_main_library_init` registers the scene-level classes with `ClassDB`. `gdlibrary.cpp` contains the older GDNative registration path; keep it aligned only when intentionally retaining that compatibility path.
- `GDN_TheWorld_MainNode` is the scene entry point. Its `init` method creates or locates `GDN_TheWorld_Globals` and `GDN_TheWorld_Viewer`, reparents them beneath the supplied world `Node3D`, and sets their owner to the edited scene/root. `GDN_TheWorld_Globals` owns configuration, logging, world status, and the asynchronous in-process client/server bridge to `TheWorld_MapManager`.
- `GDN_TheWorld_Viewer` is the runtime coordinator. Its `_process` work is split into interruptible `ProcessStep`s; it maintains world quadrants and drives visibility, mesh/material/collider updates, streaming, map UI, and editing. `QuadTree` models a quadrant and its terrain/sea chunks; `Chunk` owns the Godot mesh/collision-facing behavior. `GDN_TheWorld_Camera` and `GDN_TheWorld_Edit` provide camera and editor-facing controls.
- `Arch.txt` defines the terrain shader data contract. It uses packed quadrant-control textures plus heightmap and vertex-control texture arrays, with at most 64 ground textures, 625 quadrants per grid dimension, and four optimization-level increments. Keep CPU-side data packing, shader parameters, and these limits consistent when changing terrain representation.

## Project conventions

- Put Godot-exposed classes in the `godot` namespace, derive them with `GDCLASS`, bind script-visible methods in `_bind_methods()` using `ClassDB::bind_method`, and add them to `initialize_tw_main_module` in `register_types.cpp`. Mirror them in `gdlibrary.cpp` only if the legacy entry point must expose them too.
- Allocate scene objects with Godot allocation (`memnew`), attach them to the intended parent, and set the scene owner when they must persist in editor scenes. Follow the existing ordered shutdown protocol: `preDeinit()`, wait for `canDeinit()`, then `deinit()`; globals disconnect from the server before teardown.
- Keep Godot/godot-cpp includes inside the local `#pragma warning(push, 0)` / `#pragma warning(pop)` blocks. The project uses `pch.h`/`pch.cpp` as its precompiled-header pair.
- Preserve the viewer's interruption checks around expensive process steps and the quadrant/mesh lock-holder patterns around shared terrain data. Server work is asynchronous and replies validate `ClientServerVariant` values with `std::get_if` before use.
- Treat `plog` and `Quadtree` as vendored dependencies. Keep extension changes in `TheWorld_GDN_Viewer\` unless the task explicitly targets a dependency.
