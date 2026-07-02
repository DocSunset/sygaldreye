# hand_node

Owning package: `scene`

A hand/controller as a source node. Host: fed over HTTP (/controller →
param queue). Device: an XR pump writes the same inputs. Consumers cannot
tell which.

## Ports
- Inputs: x,y,z, qx,qy,qz,qw, trigger, grip, thumb_x, thumb_y (sliders).
- Outputs: pos (vec3), rot (quat, normalized w/ identity fallback),
  trigger, grip, thumb_x, thumb_y (scalar).
- Sources: platform param injection (HTTP) or XR runtime pump.
- Destinations: gesture nodes (wire_drag/slider_drag/dwell_delete/…),
  ui_*/poke_* widgets, anything wanting a hand.
- Temporal couplings: values written before tick (param queue / XR pump).
- Intended seams: the XR pump on device replaces HTTP without touching
  consumers — hardware-specific sourcing per the project vision.

## Requirements
- Degenerate quaternion input must yield identity, never NaN.

## Allowed dependencies
sygaldry_endpoints, Eigen.
