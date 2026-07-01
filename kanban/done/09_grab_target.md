# grab_target component

A data structure representing a point in world space that can be grabbed. Holds grab state; knows nothing about rendering or XR APIs.

## Approach

- `grab_target.hpp` declares:
  ```cpp
  struct GrabTarget {
      Eigen::Vector3f position;
      float radius;           // proximity threshold for grab detection
      bool grabbed = false;
      int grabbing_hand = -1; // 0=left, 1=right; -1 if not grabbed
      Eigen::Vector3f grab_offset; // target position minus hand position at grab start
  };
  ```
- No `.cpp` needed if the struct has no non-trivial methods. If helpers are warranted they stay under 30 lines.
- `grab_target.test.cpp`: trivial construction and field checks.

## Acceptance

- Plain data, no GL or XR headers
- `grab_target.design.md` present
