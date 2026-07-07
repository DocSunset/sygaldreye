# 23 — cube_mesh component

New component: a unit cube with per-face colors, drawable with an MVP. Narrow — only cube geometry + its draw call.

## Component
- Name: `cube_mesh`, owning package: `render`.
- Allowed dependencies: OpenGL ES, `gl_program`, Eigen.

## Ports
- Input `init()` — build VBO/VAO (24 verts, 6 face colors) and the cube shader via `gl_program`.
- Input `draw(const Eigen::Matrix4f& mvp)` — bind, set uniform, `glDrawArrays`/`glDrawElements`.
- Temporal coupling: `init()` after GL context current; `draw()` inside an eye pass with depth test enabled (item 21).

## Scope
- Header + cpp. Vertex shader: `mvp * position`; fragment: interpolated/flat per-face color.
- Per-face distinct base colors (item 01 requires different colors per face).

## Depends on
- 22 (gl_program)

## Acceptance
- A cube renders; faces visibly differ in color (verified via item 25 integration).
