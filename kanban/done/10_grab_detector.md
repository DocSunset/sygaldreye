# grab_detector component

Per-frame function that maps hand poses + grip button state to grab/release transitions on a list of `GrabTarget`s. Single concern: spatial proximity + button → semantic grab events.

## Approach

- `grab_detector.hpp` declares:
  ```cpp
  struct HandState {
      Eigen::Vector3f position;
      bool valid;
      bool grip_pressed;
  };

  void update_grabs(std::span<HandState const> hands,
                    std::span<GrabTarget> targets);
  ```
- `grab_detector.cpp` implements: for each hand that just pressed grip, find the closest ungrabbbed target within its radius and set `grabbed = true`, `grabbing_hand`, `grab_offset`. For hands that released grip, clear grab state on their target.
- No XR headers; hand poses are passed as plain `HandState`. The `input` component's `HandPose` can be mapped to `HandState` at the call site.
- `grab_detector.test.cpp`: test grab acquisition, release, and that a hand cannot grab two targets simultaneously.

## Acceptance

- Host-buildable and host-testable (no GL, no XR)
- Grab and release transitions are correct (tests pass)
- `grab_detector.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `grab_target` (item 05)
