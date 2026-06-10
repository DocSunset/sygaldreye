# fly_camera

Owning package: `render` (shared math)

Pure camera math: pos/yaw/pitch/fov → view/proj matrices. No GL, no input.

## Ports
- Inputs: struct fields (pos, yaw, pitch, fov); `proj(aspect)` parameter.
- Outputs: `view()`, `proj()`, `forward()`, `right()` matrices/vectors.
- Sources/Destinations/Temporal couplings: none (stateless value type).
- Intended seams: consumed by fly_camera_node (graph) and host_input
  (windowed WASD); any future platform reuses the same math.

## Requirements
- Right-handed, -Z forward, column-major Eigen; matches Quest conventions.

## Allowed dependencies
Eigen only.
