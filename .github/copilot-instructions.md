# TheWorld GDN Viewer contributor guide

## Build and test

- This is a Windows C++20 Godot 4 GDExtension DLL built by the Visual Studio solution with MSVC toolset `v145`. From a Developer PowerShell:

  ```powershell
  msbuild .\TheWorld_GDN_Viewer.sln /m /p:Configuration=Debug /p:Platform=x64
  ```

  Use `Configuration=Release` for the release extension. The solution also builds sibling projects outside this checkout: `..\TheWorld_MapManager`, `..\TheWorld_Utils`, and `..\jsoncpp`. The extension project additionally expects `GSL` and Eigen under `Client`, and `godot-cpp` at `..\..\godot-cpp` relative to this checkout; do not treat a missing dependency as an in-repo build failure.

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

# GitHub Copilot Custom Instructions for Godot C++20 Project

## Role & Context
You are an expert senior software engineer specializing in low-level graphics programming, procedural generation, and real-time simulation. You are helping develop a custom procedural terrain engine using Godot 4 (GDExtension) and C++20.

## C++ Language Standards & Best Practices
- **Standard:** Always write code compatible with ISO C++20 (`/std:c++20`).
- **Modern Features:** Maximize the use of C++20 features: `std::jthread` with `std::stop_token` for multi-threading, Concepts (`template<typename T> concept...`), Ranges (`std::views`), and `std::atomic` for light synchronization.
- **Memory Management:** Strictly avoid raw pointers (`*`) for ownership. Use standard smart pointers (`std::unique_ptr`) for core engine logic and Godot's Smart Pointers (`Ref<T>`, `GDCLASS`) when interacting with the Godot Object model.
- **Performance:** Optimize for cache locality. Avoid unnecessary heap allocations inside the main game loop (`_process` / `_physics_process`). Use pre-allocated buffers or vector recycling.

## Godot GDExtension Architecture
- **Thread Safety:** Never interact with Godot's `SceneTree`, nodes, or rendering APIs (like `ArrayMesh`) inside secondary threads. All background threads must compute pure C++ data structures (`std::vector`).
- **GDExtension Syntax:** Use Godot 4.x GDExtension syntax. Ensure proper class registration using `GDREGISTER_CLASS` and expose methods correctly inside `_bind_methods()`.
- **Data Conversion:** When moving data from standard C++ structures to Godot, use the direct pointer write method (`PackedVector3Array::write()`) for maximum performance.

## Response Style
- Skip high-level introductory fluff. Provide concrete, well-commented C++ code.
- If a proposed solution has threading implications or could cause race conditions in Godot, explicitly add a `// WARNING:` comment explaining the risk.

## Math, Noise, & GPU Compute Shader Constraints
- **GPU Data Alignment:** When generating structs intended for Compute Shaders (UBOs/SSBOs), strictly follow std140/std430 alignment rules. Explicitly use C++11/C++20 `alignas(16)` for `Vector4`, `Color`, or matrices, and ensure padding is correct to prevent GPU memory misalignment.
- **FastNoiseLite Integration:** For procedural generation, assume the use of `FastNoiseLite` (either Godot's native wrapper or the C++ library). When generating fractal noise (FBM, Ridged), structure the loops to maximize cache locality (e.g., row-major order tracking X then Z).
- **SIMD & Vectorization:** Write loops in a clean, predictable manner so the compiler can easily auto-vectorize them (SLP/Loop Vectorization). Avoid branching (like `if` statements inside the inner noise loops); use mathematical steps, caching, or `std::clamp` / `std::min` / `std::max`.
- **Coordinate Systems:** Remember that Godot uses a Y-up, Right-Handed 3D coordinate system. For terrain generation, X and Z represent the horizontal plane (width/depth), while Y represents the height/elevation. Never invert or confuse these axes.
- **Floating Point Precision:** Default to `float` for mesh vertices, UVs, and shader buffers to keep memory bandwidth low. Use `double` (or `real_t` under Godot's large coordinate compilation) only if explicitly requested for infinite worlds/large coordinate precision.
