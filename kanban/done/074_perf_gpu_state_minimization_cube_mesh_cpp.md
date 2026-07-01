# Hoist glUseProgram out of draw(); remove redundant VAO unbind

**File:** `components/cube_mesh/cube_mesh.cpp`
**Principle:** Minimize GPU state changes by hoisting redundant calls out of inner loops.

`CubeMesh::draw()` (lines 335-341) calls `prog_.use()` (i.e., `glUseProgram`) and `glBindVertexArray(vao_)` for every cube, and then unbinds the VAO with `glBindVertexArray(0)` at the end. With 1-3 cubes × 2 eyes = 2-6 draw calls per frame, this generates 4-12 redundant GPU state changes. The program and VAO are the same object every call; there is no reason to rebind them. The `glBindVertexArray(0)` unbind at the end is purely defensive and unnecessary since the VAO will be rebound next draw.

Expose `bind()` and `unbind()` methods (or `begin_batch()`/`end_batch()`) on `CubeMesh` so the caller can call `glUseProgram` and `glBindVertexArray` once before the cube loop and `glBindVertexArray(0)` once after. Alternatively, move program binding into `Renderer::render_eyes()` so `draw()` only sets the uniform and issues the draw call.

## Acceptance
- `glUseProgram` is called at most once per eye per frame regardless of cube count.
- `glBindVertexArray(vao_)` is called at most once per eye per frame.
- `glBindVertexArray(0)` is called at most once after the cube loop (not inside it).
- Cubes still render correctly.
