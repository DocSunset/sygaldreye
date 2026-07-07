# Remove gl_program.hpp (and its GL dependency) from cube_mesh.hpp

**File:** `components/cube_mesh/cube_mesh.hpp`
**Principle:** Headers should include only what is needed to express the public API; implementation-only includes belong in .cpp files.

`cube_mesh.hpp` includes `gl_program.hpp` (line 3), which itself includes `GLES3/gl3.h`. The `GlProgram prog_` member is private, so its full type is only needed in `cube_mesh.cpp`. Any consumer of `cube_mesh.hpp` transitively pulls in GL headers even if it never touches GL. The public API (`init()`, `draw(const Eigen::Matrix4f&)`) requires only `Eigen/Core`.

## Acceptance
- `cube_mesh.hpp` does not include `gl_program.hpp`.
- `GlProgram` is forward-declared in `cube_mesh.hpp` (or the member is stored via pointer/opaque type) so the header compiles without GL.
- `gl_program.hpp` is included only in `cube_mesh.cpp`.
- The build still succeeds.
