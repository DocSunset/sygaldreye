# sky_dome

Large inverted sphere drawn first each frame with depth write/test disabled, providing a sky gradient, optional sun/moon disc, and optional star field.

## Ports

### Inputs
- `SkyParams::horizon_color`, `zenith_color` — gradient endpoint colours
- `SkyParams::radius` — dome radius (default 500 m)
- `SkyParams::body_dir`, `body_color`, `body_angular_radius` — sun/moon disc; disabled when `body_color.a == 0`
- `SkyParams::star_count`, `enable_stars` — star field toggle and density

### Outputs
- Rendered sky geometry submitted to the current OpenGL ES framebuffer

### Sources
- `draw(vp)` — view-projection matrix from `xr_session` / renderer frame loop

### Destinations
- Must be called before any opaque scene geometry each frame

### Temporal Couplings
- `create()` must be called with a valid EGL/GLES context before `draw()`

### Intended Seams
- `set_params()` allows per-frame parameter updates (e.g. animated day/night cycle)

## Requirements
- Sky gradient transitions smoothly from horizon (y=0) to zenith (y=1)
- Sun/moon disc rendered with soft edge via `smoothstep`
- Star field is deterministic (fixed seed), generated once at `create()`
- Depth write disabled and depth test disabled during draw; both restored afterward
- Dome faces point inward (vertex shader uses `w=0` trick to pin to far plane)

## Allowed Dependencies
- `sphere_geometry`
- `sphere_mesh`
- `gl_program`
- GLES 3.0, Eigen, Android log

## Owning Package
`scene`
