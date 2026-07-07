# Test that update() after set_controller_poses() erases hand cubes

**File:** `components/scene/scene.cpp`
**Principle:** All orderings and combinations of mutating methods should be explicitly tested; undefined ordering silently corrupts state.

`update()` calls `cubes_.resize(1)` unconditionally, which silently discards any controller cubes appended by a prior `set_controller_poses()` call. In `app.cpp` the calls are currently ordered `update()` then `set_controller_poses()`, which is correct — but this ordering constraint is nowhere documented or enforced, and a future refactor could reverse it. The existing `scene.test.cpp` has no test for `set_controller_poses()` at all, so the ordering bug would go undetected. Tests should cover both orderings and verify the resulting cube count.

## Acceptance
- A test calls `update()` then `set_controller_poses()` with both hands valid and asserts `cubes().size() == 3`
- A test calls `set_controller_poses()` then `update()` and documents (with a comment or EXPECT) that this ordering yields only 1 cube, demonstrating the ordering hazard
- A test covers `set_controller_poses()` with left-only, right-only, and neither valid, asserting the correct cube count each time
