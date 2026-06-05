# billboard_quad

Camera-facing instanced quad renderer. Draws a set of billboards given per-instance position, size, and color.

## Ports

### Inputs
- `set_instances(span<BillboardInstance const>)` — replaces the current instance list and uploads to GPU.

### Outputs
- `draw(vp, camera_right, camera_up)` — submits one instanced draw call.

### Sources
- Caller provides the view-projection matrix and camera basis vectors each frame.

### Destinations
- Writes to the currently bound OpenGL ES framebuffer.

### Temporal couplings
- `create` must be called before `set_instances` or `draw`.
- A valid OpenGL ES context must be current on the calling thread for all methods.

### Intended seams
- None.

## Requirements

- Render up to `max_instances` billboards per draw call via `glDrawArraysInstanced`.
- `set_instances` with zero instances must produce no draw call.
- Quads always face the camera (billboarding) using the supplied `camera_right` and `camera_up` vectors.
- `size` represents world-space half-extents (width, height).

## Allowed dependencies

- `gl_program`

## Owning package

`render`
