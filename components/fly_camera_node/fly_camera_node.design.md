# fly_camera_node

Owning package: `scene`

The camera as a graph node. The platform renders with `outputs.pv`;
anything can patch into the pose inputs.

## Ports
- Inputs: x,y,z,yaw,pitch,fov (sliders, patchable), aspect (slider,
  platform-pumped per frame).
- Outputs: view, proj, pv (mat4), pos (vec3), yaw, pitch (scalar).
- Sources: platform writes `inputs.aspect` directly each frame.
- Destinations: platform reads `<id>.pv` from the graph value store; the
  conventional id is `camera`.
- Temporal couplings: aspect pump happens before tick; pv consumed after.
- Intended seams: multiple instances allowed; the platform uses the one
  named `camera` (convention pending a real camera-selection design).

## Requirements
- Header-only; no GL; pure function of inputs.

## Allowed dependencies
fly_camera, sygaldry_endpoints, Eigen.
