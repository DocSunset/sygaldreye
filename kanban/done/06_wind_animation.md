# wind_animation component

A vertex-shader-based wind effect that displaces mesh vertices by a time-varying noise field, simulating foliage sway. Applied to grass, tree canopies, and any mesh that should respond to wind.

## Approach

- Extend `flat_shader` (or create a parallel `wind_shader`) with additional uniforms:
  - `float time`
  - `vec3 wind_dir` — normalised dominant wind direction
  - `float wind_strength`
  - `float sway_mask` — per-vertex attribute (0=root/anchored, 1=tip/free); stored in the W component of the normal or a dedicated attrib
- Vertex displacement: `offset = wind_dir * wind_strength * sway_mask * sin(time * freq + dot(position.xz, wind_dir.xz) * spatial_freq)`. The `sin` keeps it cheap; spatial frequency creates a wave-across-the-field look.
- `wind_shader.hpp` / `wind_shader.cpp`: same structure as `flat_shader` plus the new uniform setters.
- `wind_animation.hpp` declares helpers to stamp `sway_mask` values into a `TriMeshData` given a height threshold (vertices above threshold get mask=1).

## Acceptance

- A flat grass patch visibly sways in a wave pattern (on-device)
- `sway_mask = 0` on all vertices produces zero displacement
- `wind_shader.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `flat_shader` (item 02) — shares most code; may refactor into a common base
- `scene_time` (item 05)
