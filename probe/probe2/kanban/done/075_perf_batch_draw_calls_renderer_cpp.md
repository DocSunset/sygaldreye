# Batch cube draw calls with instanced rendering instead of per-cube draw loop

**File:** `components/renderer/renderer.cpp`
**Principle:** Batch similar draw calls and sort by material/mesh to minimize per-draw overhead; plan for instanced rendering.

`render_eyes()` (lines 209-212) issues one `CubeMesh::draw()` call per cube per eye. With the current 1-3 cubes this is 2-6 draw calls/frame; each carries a full GPU state validation cost (`glUseProgram`, `glBindVertexArray`, uniform upload). The mesh and shader are the same for all cubes — only the MVP matrix differs. This is the canonical case for instanced rendering: upload all MVPs in a UBO or per-instance VBO, then issue a single `glDrawElementsInstanced` call per eye.

Add a `draw_instanced(std::span<const Eigen::Matrix4f> mvps)` method to `CubeMesh` that uploads the MVP array to a per-instance buffer and calls `glDrawElementsInstanced`. Update `render_eyes()` to build the MVP array for all cubes before calling `draw_instanced` once per eye.

## Acceptance
- Only one `glDrawElements`/`glDrawElementsInstanced` call is issued per eye per frame regardless of cube count.
- `glUseProgram` and `glBindVertexArray` are called at most once per eye per frame.
- Rendered output is visually identical to the current per-cube loop.
