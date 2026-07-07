# scene

Owning package: `scene`

## Ports

- **Inputs**: `update(double time_s)` — advance animation; `time_s` is seconds since XR session start, sourced from `XrFrameState::predictedDisplayTime` (nanoseconds) converted via `* 1e-9`; monotonically increasing at roughly wall-clock rate; `set_controller_poses()` — placeholder for item 28
- **Outputs**: `cubes()` — current set of cube instances to draw (valid until next `update`)
- **Temporal couplings**: caller must call `update` before reading `cubes` each frame

## Requirements

- Maintain a single rotating cube at world position (0, 1.5, -2)
- Cube rotates around Y axis at 1 rad/s
- No dependency on XR or GL internals

## Allowed dependencies

- Eigen (math only)

## Intended seams

- `set_controller_poses` signature is a placeholder; item 28 extends it without modifying scene internals
