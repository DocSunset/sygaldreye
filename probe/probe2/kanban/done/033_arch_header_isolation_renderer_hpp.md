# Remove scene.hpp and cube_mesh.hpp from renderer.hpp

**File:** `components/renderer/renderer.hpp`
**Principle:** Headers should include only what is needed to express the public API; implementation-only includes belong in .cpp files.

`renderer.hpp` includes `scene.hpp` (line 12) and `cube_mesh.hpp` (line 13). `scene.hpp` is only needed because `render_eyes` takes `std::span<const CubeInstance>` — `CubeInstance` could be forward-declared or defined in a thinner shared header. `CubeMesh` is a private member and its full definition is only needed in `renderer.cpp`. Any consumer of `renderer.hpp` transitively includes all of `scene` and `cube_mesh`, including their GL and Eigen dependencies.

## Acceptance
- `renderer.hpp` does not include `scene.hpp` or `cube_mesh.hpp`.
- `CubeInstance` is either forward-declared in a shared header or moved so `render_eyes`'s parameter type compiles without pulling in all of `scene.hpp`.
- `cube_mesh.hpp` is included only in `renderer.cpp`.
- The build still succeeds.
