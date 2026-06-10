# rubber_band_controller

A composited VR widget: a fixed anchor sphere connected by a rubber-band cylinder to a freely draggable control sphere. Exposes the offset vector between them.

## Ports

### Inputs
- `create(anchor_pos, label_fn)` — initial anchor world position; optional label formatting function
- `update(hands)` — per-frame hand state driving grab detection and position updates

### Outputs
- `draw(view_proj)` — emits all draw calls for this frame
- `offset()` — vector from anchor to control sphere

### Sources
- `HandState` array from `grab_detector`

### Destinations
- GL context must be active when `create()` and `draw()` are called

### Temporal couplings
- `create()` must be called after GL context is initialised
- `update()` must be called before `draw()` each frame

### Intended seams
- `label_fn` — injected callable transforms the offset vector to a display string; default shows magnitude

## Requirements

- Grabbing the anchor moves both spheres (constant relative offset)
- Grabbing the control moves only the control
- `offset()` returns the exact world-space vector from anchor to control
- Releasing either sphere holds last position
- Label renders below the anchor sphere each frame via the injected `label_fn`

## Allowed dependencies

- `grab_target`
- `grab_detector`
- `sphere_mesh`, `sphere_geometry`
- `cylinder_mesh`
- `rgba_shader`
- `text_mesh`
- `gl_program` (for destructor completeness)
- Eigen

## Owning package

`scene`
