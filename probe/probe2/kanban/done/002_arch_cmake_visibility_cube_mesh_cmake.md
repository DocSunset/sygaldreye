# Fix cmake visibility: cube_mesh should PRIVATE-link GLESv3, log, and gl_program

**File:** `components/cube_mesh/CMakeLists.txt`
**Principle:** CMake target dependencies must use PRIVATE unless the dependency's types appear in the component's public header; PUBLIC linkage leaks transitive dependencies to all consumers.

`cube_mesh/CMakeLists.txt` links `GLESv3`, `log`, and `gl_program` as PUBLIC. None of these appear in `cube_mesh.hpp`'s public interface — `GLuint` types from `gl_program.hpp` are only used in the private `prog_`, `vao_`, and `vbo_` members, and GL/log calls are entirely in `cube_mesh.cpp`. Any consumer of `cube_mesh` silently inherits GL and log linkage without needing it.

## Acceptance
- `GLESv3`, `log`, and `gl_program` are linked PRIVATE in `components/cube_mesh/CMakeLists.txt`.
- The build still succeeds.
- No unintended consumers of `cube_mesh` gain or lose linkage as a visible side-effect.
