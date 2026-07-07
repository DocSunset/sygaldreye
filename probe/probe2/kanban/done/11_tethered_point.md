# tethered_point component

Pure math: given an anchor position and a desired position, return the nearest position to the desired position that lies within [min_dist, max_dist] of the anchor. Intended as a building block for specialized `rubber_band_controller` wrappers that constrain the offset (e.g. a distance-clamped or axis-projected slider); not used by the basic rubber band itself.

## Approach

- `tethered_point.hpp` declares:
  ```cpp
  Eigen::Vector3f tethered_point(Eigen::Vector3f anchor,
                                  Eigen::Vector3f desired,
                                  float min_dist,
                                  float max_dist);
  ```
- `tethered_point.cpp`: compute distance; if within bounds return `desired`; else return `anchor + direction * clamped_dist`. Handle the degenerate case where `desired == anchor` (pick an arbitrary direction, e.g. +X).
- `tethered_point.test.cpp`: within range (passthrough), too close (pushed out to min_dist), too far (pulled in to max_dist), coincident (doesn't crash).

## Acceptance

- All four test cases pass
- Host-buildable, no GL or XR headers
- `tethered_point.design.md` present; `.cpp` under 50 lines of substantive code
