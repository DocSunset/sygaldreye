# water_surface

Animated low-poly water surface: a grid mesh with per-frame height animation via two crossed sine waves, flat-shaded with depth-based color lerp.

## Ports

**Inputs:**
- `WaterParams` at construction time (grid dimensions, cell size, wave amplitude/speed, colors)
- `float time_s` passed to `update()` each frame

**Outputs:**
- Draw calls emitted by `draw(mvp)`

**Sources:** none

**Destinations:**
- Caller must enable `GL_BLEND` before `draw()` and disable after (alpha < 1)

**Temporal couplings:**
- `create()` must be called with an active GL context
- `update()` and `draw()` must be called with the same active GL context

**Intended seams:**
- `WaterParams` fully parameterizes appearance; `mesh_data()` exposes geometry for testing

## Requirements

- Grid of `grid_w × grid_h` vertices in XZ; Y animated each frame
- Height: `h = wave_height * (sin(x*0.3 + time*speed) + sin(z*0.5 + time*0.8*speed)) * 0.5`
- Per-face normals recomputed from cross product after height update
- Color lerped between `shallow_color` and `deep_color` by normalized wave height
- No blend-state management; caller's responsibility

## Allowed Dependencies

- `tri_mesh`
- `gl_program`

## Owning Package

`scene`
