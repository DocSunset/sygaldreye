# Add edge-case tests for projection() and view() in vr_math

**File:** `components/vr_math/vr_math.cpp`
**Principle:** Mathematical/algorithmic functions should be tested at boundary and extreme values, not just the nominal case.

`vr_math.test.cpp` currently tests only a symmetric 45°/eye FOV and a 90° Y-axis rotation. `projection()` is untested with asymmetric FOV (left ≠ right, up ≠ down), near-zero angles (approaching degenerate frustum), and extreme angles (±85° as seen on Quest 3 at the horizontal extents). `view()` is untested with a non-identity position + rotation combined (the most common real case). These gaps mean coordinate frame bugs, sign errors in the asymmetric terms `(r+l)/(r-l)` and `(t+b)/(t-b)`, and translation errors in `view()` would not be caught.

## Acceptance
- Test: asymmetric FOV (e.g. angleLeft=-80°, angleRight=40°, angleDown=-50°, angleUp=60°) — verify `m(0,2)` and `m(1,2)` are non-zero with correct sign
- Test: extreme symmetric FOV (±85°) — verify matrix is finite and `m(0,0)` > 0
- Test: `view()` with non-zero position and 90° Y rotation — verify the eye translation column is `-(R^T * t)`
- Test: `pose_to_world()` with non-zero position — verify round-trip with `view()` produces identity (up to float tolerance)
