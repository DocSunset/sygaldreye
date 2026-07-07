# flat_shader component

A GL shader program that renders geometry with per-vertex color using flat (face-constant) interpolation. This shader defines the low-poly visual style: each triangle displays a single solid color with no gradient blending across its surface.

## Approach

- Vertex shader: transform position by MVP, pass `color` as a `flat out` varying (GLSL `flat` keyword forces provoking-vertex interpolation).
- Fragment shader: output the `flat in` color directly. No lighting computation — the art style carries the visual weight.
- `flat_shader.hpp` declares `FlatShader` with `create()`, `use()`, and `set_mvp(Eigen::Matrix4f const&)`.
- Color is encoded as a per-vertex attribute (vec4 RGBA), not a uniform, so a single draw call can render a multi-colored mesh.
- `flat_shader.cpp` embeds GLSL as string literals, compiles, links, caches the `mvp` uniform location.

## Acceptance

- A mesh with alternating red/blue/green per-face colors renders visibly distinct flat-colored triangles (verify on-device)
- No GL errors on shader compile/link
- `flat_shader.design.md` present; `.cpp` under 100 lines of substantive code
