# input

Owning package: `xr`

## Ports

**Sources** (coupled inputs from other components):
- `XrInstance` from `xr_session` — needed to create action set and string paths
- `XrSession` from `xr_session` — needed to attach action sets and create spaces
- `XrSpace` (world space) from `xr_session` — reference space for locating hand spaces
- `XrTime` (predicted display time) from `xr_session` — passed to `xrLocateSpace`

**Inputs** (own, uncoupled):
- `create(instance, session)` — one-time setup; call after session is created
- `sync(session, worldSpace, time)` — called each frame inside the frame loop

**Outputs:**
- `hand_pose(hand)` → `HandPose{XrPosef, bool valid}` — current grip pose for left (0) or right (1) hand

**Temporal couplings:**
- `create` must be called before `sync`
- `sync` must be called between `xrBeginFrame` and `xrEndFrame` (OpenXR requirement for `xrSyncActions`)

**Intended seams:** none

## Requirements

- Track left and right controller grip poses each frame via OpenXR action system
- Expose validity flag so callers can skip rendering absent controllers
- Log first valid pose for debug visibility

## Allowed dependencies

- OpenXR loader (`openxr_loader`)
- Android log (`log`)
