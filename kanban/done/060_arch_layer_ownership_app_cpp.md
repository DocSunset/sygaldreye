# Move hand transform computation out of app.cpp into scene

**File:** `components/app/app.cpp`
**Principle:** Each layer should own its domain's logic; transform computation is a scene concern, not a platform/orchestration concern.

`app.cpp` (lines 63–82) computes hand cube model matrices — including scale, local offset, and world transform — then passes the results into `scene_.set_controller_poses()`. This mixes scene content logic (what the hand cube looks like and where it sits relative to the grip) with platform orchestration (polling input and driving the render loop). The constants `HAND_CUBE_OFFSET_X/Y/Z_CM` and the `scale_m` computation belong in `scene` or a dedicated hand-cube component, not in `android_main`.

## Acceptance
- `app.cpp` passes only the raw `XrPosef` (or `HandPose`) to the scene; it does not compute model matrices.
- `Scene` (or a new child component) owns the scale and offset constants and constructs the final model matrix internally.
- `app.cpp`'s frame-loop lambda contains no Eigen matrix math.
- The build succeeds and hand cubes render correctly on-device.
