# Cache uniform location at link time; expose location-based uniform setter

**File:** `components/gl_program/gl_program.cpp`
**Principle:** Minimize GPU state changes by eliminating redundant driver calls; cache what can be cached at init time.

`GlProgram::uniform(const char* name, ...)` (lines 392-399) calls `glGetUniformLocation(id, name)` on every draw call. This is a string-table lookup in the driver, called 2-6 times per frame (once per cube per eye) with the same string `"uMVP"`. Uniform locations are fixed after link and need only be queried once.

After `glLinkProgram` succeeds in `GlProgram::build()`, query and store the location of known uniforms (or provide a `cache_uniform(const char* name) -> GLint` method). Add an overload `uniform(GLint loc, const Eigen::Matrix4f&)` that takes the pre-cached location directly, and update `CubeMesh::draw()` to use the cached location.

## Acceptance
- `glGetUniformLocation` is never called during the per-frame draw path.
- The uniform location for `uMVP` is stored at shader build time and reused each frame.
- A new `uniform(GLint loc, const Eigen::Matrix4f&)` overload (or equivalent) is present.
