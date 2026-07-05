# vr_math

Owning package: `render`

Pure math utilities converting OpenXR pose/fov types to Eigen matrices for use in OpenGL ES rendering.

## Ports

- Inputs: `XrFovf` (per-eye field of view angles), `XrPosef` (pose in world space), near/far clip distances
- Outputs: `Eigen::Matrix4f` projection, view, and model matrices
- Sources: none
- Destinations: none (results consumed by renderer draw calls)
- Temporal couplings: none
- Intended seams: none

## Requirements

- `projection(fov, near_z, far_z)` — asymmetric perspective matrix from per-eye angles, OpenGL right-handed convention, NDC z in [-1, 1]
- `view(pose)` — inverse of the eye world pose, suitable as GL view matrix
- `pose_to_world(pose)` — forward TR matrix placing content at a given pose

## Allowed dependencies

- Eigen (math)
- OpenXR headers (types only, no API calls)
