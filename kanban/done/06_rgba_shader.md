# rgba_shader component

A GL shader program that renders geometry with a uniform RGBA color, supporting alpha blending. Needed for the translucent anchor sphere and the inner fill sphere.

## Approach

- Vertex shader: transform position by MVP, pass nothing else.
- Fragment shader: output a uniform `vec4 color` directly.
- `rgba_shader.hpp` declares `RgbaShader` with `create()`, `use()`, and setters for `mvp` (`mat4`) and `color` (`vec4`).
- `rgba_shader.cpp` compiles and links the two GLSL sources (embedded as string literals), caches uniform locations.
- Callers are responsible for enabling/disabling `GL_BLEND` and setting blend factors around draw calls that need transparency.

## Acceptance

- Shader compiles and links without errors on a connected device (check logcat)
- Setting `color.a < 1.0` with `GL_BLEND` enabled produces visible transparency
- `rgba_shader.design.md` present; each `.cpp` under 100 lines of substantive code
