# grab_target

Owning package: `scene`

Pure data structure representing a point in world space that can be grabbed by a hand controller.

## Ports

**Inputs** (own, uncoupled):
- `position` — world-space center point where the grab target is located
- `radius` — proximity threshold for grab detection

**Outputs:**
- `grabbed` — boolean indicating whether a hand is currently holding this target
- `grabbing_hand` — which hand has grabbed this target (0=left, 1=right, -1=not grabbed)
- `grab_offset` — target position minus hand position at grab start; used to maintain relative offset during drag

**Sources:** none

**Destinations:** none (results consumed by scene logic and rendering)

**Temporal couplings:** none

**Intended seams:** none

## Requirements

- Default-constructed grab target is ungrabbed with zero position and 5cm radius
- Support storing hand index and offset for multi-target grab logic
- No GL, XR, or rendering dependencies; pure data

## Allowed dependencies

- Eigen (math)
