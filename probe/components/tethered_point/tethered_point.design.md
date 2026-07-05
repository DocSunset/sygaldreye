# tethered_point

Owning package: `scene`

Pure math component implementing distance-clamped point projection. Given an anchor position and a desired position, returns the nearest point to the desired position that lies within a specified distance range from the anchor.

## Ports

- Inputs: anchor position (Vector3f), desired position (Vector3f), min_dist (float), max_dist (float)
- Outputs: clamped position (Vector3f)
- Sources: none
- Destinations: none
- Temporal couplings: none
- Intended seams: none

## Requirements

- `tethered_point(anchor, desired, min_dist, max_dist)` — returns a point that is:
  - `desired` if distance from anchor to desired is within [min_dist, max_dist]
  - `anchor + min_dist * direction` if distance < min_dist
  - `anchor + max_dist * direction` if distance > max_dist
  - Degenerate case (anchor == desired): returns `anchor + min_dist * UnitX()` to avoid division by zero

## Allowed dependencies

- Eigen (math)
- standard library (cmath)
