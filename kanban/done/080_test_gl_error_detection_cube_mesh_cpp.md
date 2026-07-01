# Add glGetError checks after GL calls in CubeMesh::init and draw

**File:** `components/cube_mesh/cube_mesh.cpp`
**Principle:** Wrapping every GL call with a glGetError check prevents silent error accumulation and makes failures detectable by tests.

`CubeMesh::init()` issues `glGenVertexArrays`, `glGenBuffers`, `glBindVertexArray`, `glBindBuffer`, `glBufferData`, `glEnableVertexAttribArray`, and `glVertexAttribPointer` with no `glGetError` checks. `draw()` issues `glBindVertexArray` and `glDrawElements` with no checks. If any of these fail (e.g., wrong attribute stride, out-of-memory for buffers), the error queue accumulates silently. A shared `GL_CHECK` macro (defined once, e.g. in a gl_check.hpp header) wrapping each call would surface these errors immediately with file and line context.

## Acceptance
- All GL calls in `CubeMesh::init()` and `CubeMesh::draw()` are wrapped with `GL_CHECK` or equivalent
- `GL_CHECK` logs the GL error code with file and line if `glGetError()` returns non-zero
- No new GL errors are swallowed silently
