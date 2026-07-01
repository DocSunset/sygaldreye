# Fix cmake visibility: renderer should PRIVATE-link scene and cube_mesh

**File:** `components/renderer/CMakeLists.txt`
**Principle:** CMake target dependencies must use PRIVATE unless the dependency's types appear in the component's public header; PUBLIC linkage leaks transitive dependencies to all consumers.

`renderer/CMakeLists.txt` links `scene` and `cube_mesh` as PUBLIC, but neither type is exposed in `renderer.hpp`'s public API — `CubeInstance` (from `scene.hpp`) and `CubeMesh` (from `cube_mesh.hpp`) only appear in `renderer.cpp`. This causes every consumer of `renderer` to silently acquire the entire `scene` and `cube_mesh` include graphs. Similarly, `EGL`, `GLESv3`, and `log` are implementation-only and should be PRIVATE.

## Acceptance
- `target_link_libraries(renderer ...)` in `components/renderer/CMakeLists.txt` links `scene`, `cube_mesh`, `EGL`, `GLESv3`, and `log` as PRIVATE.
- The build still succeeds.
- `app/CMakeLists.txt` does not need to add any new explicit dependencies as a result of this change (i.e., `app` already lists what it needs directly).
