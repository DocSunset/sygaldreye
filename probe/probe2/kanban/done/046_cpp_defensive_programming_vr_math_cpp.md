# Add precondition assertions to projection() to guard against division by zero

**File:** `components/vr_math/vr_math.cpp`
**Principle:** Function preconditions must be enforced at the call site via assertions or early-return guards so that invalid inputs produce a clear diagnostic rather than silent NaN/inf propagation.

`projection()` computes `2.f * near_z / (r - l)`, `2.f * near_z / (t - b)`, and `-(far_z + near_z) / (far_z - near_z)` without checking that the denominators are non-zero or that `near_z > 0`. If the FOV is degenerate or `near_z == far_z`, all four matrix entries become NaN or inf, silently corrupting every MVP matrix for that frame. Add `assert(near_z > 0.f && far_z > near_z)` and `assert(r != l && t != b)` at the top of the function.

## Acceptance
- `projection()` asserts `near_z > 0.f`, `far_z > near_z`, `r != l`, and `t != b` before performing any division
- Assertions use `<cassert>` (already available or added)
- Build succeeds with no new warnings; existing callers pass valid values and assertions do not fire at runtime
