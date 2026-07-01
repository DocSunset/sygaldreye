# Make renderer geometry-agnostic: move draw-call dispatch out of render_eyes

**File:** `components/renderer/renderer.cpp`
**Principle:** Each component should own one concern; the renderer should manage the frame/eye loop and GL context, not know about specific geometry types.

`Renderer::render_eyes` (lines 158–238) takes `std::span<const CubeInstance>` and directly iterates cubes to compute MVP and issue draw calls (lines 207–212). This couples the renderer to the cube geometry type. Adding a new geometry type (e.g. a quad overlay) requires modifying `renderer.cpp`. The renderer should instead expose a per-eye callback or context object that lets the caller issue its own draw calls.

## Acceptance
- `render_eyes` (or a replacement API) no longer references `CubeInstance` or `CubeMesh`.
- The renderer exposes a `begin_eye` / `end_eye` pattern or a per-eye callback that yields projection and view matrices, and the caller issues draw calls inside it.
- Scene/app code calls `cube_mesh_.draw(mvp)` directly in the callback, not inside the renderer.
- The build succeeds and cubes render correctly on-device.
