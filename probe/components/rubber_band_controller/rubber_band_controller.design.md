# rubber_band_controller

A composited VR widget as a graph node (`rubber_band`): a fixed anchor sphere
connected by a rubber-band cylinder to a freely draggable control sphere. Two
hand poses + grips drive grab detection internally; the offset vector and a
declarative Mesh+Surface flow out. GL lives only in render_region (ABI v8).

## Ports

### Inputs
- `left_pos`, `right_pos` (vec3) — hand world positions.
- `left_grip`, `right_grip` (0/1) — grip pressed.
- `anchor_x`/`y`/`z` — initial anchor placement (params, seeded on first tick).

### Outputs
- `offset` (vec3) — world-space vector from anchor to control sphere.
- `surface`+`mesh` — both spheres (gold anchor, blue control) and the red
  band as one lit per-vertex-color mesh, for a `draw` node.

### Sources / Destinations
- Hand poses come from `hand`/controller nodes via edges; `surface`/`mesh`
  feed a `draw` node; `offset` feeds any vec3 consumer.

### Temporal couplings
- Anchor is seeded from the params on the first tick.

### Intended seams
- Collision/grab logic is `grab_detector`; geometry is `sphere_geometry` +
  `cylinder_mesh`. The label the old GL widget drew is dropped — text is a
  `text_label` node's job now.

## Requirements

- Grabbing the anchor moves both spheres (constant relative offset).
- Grabbing the control moves only the control.
- `offset` is the exact world-space vector from anchor to control.
- Releasing either sphere holds last position.

## Allowed dependencies

- `grab_target`, `grab_detector`
- `sphere_geometry`, `cylinder_mesh`
- `tri_mesh`, `render_payloads`, `sygaldry_endpoints`
- `eyeballs_node_abi` (plugin export only)
- Eigen

## Owning package

`scene`
