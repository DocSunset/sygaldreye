# Extract hand-cube model matrix computation into a pure free function

**File:** `components/app/app.cpp`
**Principle:** Pure functions (those with no I/O or mutable state) are intrinsically testable; extracting them enables offline unit tests without hardware.

The hand cube matrix chain (lines 66–82 of `android_main`'s frame lambda) — `pose_to_world(pose) * local_T * scale_m` — is computed inline inside the NativeActivity frame loop. This logic depends only on a pose, an offset, and a scale, yet it is impossible to test because it is embedded in a lambda that requires an active XR session. Extracting it as a free function `hand_cube_model(const XrPosef&, float offset_x, float offset_y, float offset_z, float scale) -> Eigen::Matrix4f` makes it testable offline with the same math infrastructure used by vr_math.test.cpp.

## Acceptance
- A pure free function exists that computes the hand cube model matrix from a pose, per-axis offsets, and a uniform scale
- `android_main` calls this function instead of inlining the computation
- A test covers: identity pose at zero offset produces a scaled identity; non-zero offset shifts the translation column correctly
