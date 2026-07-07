# Document or enforce span invalidation contract on Scene::cubes()

**File:** `components/scene/scene.hpp`
**Principle:** Spans and pointer-based views must express their validity window; callers must not be able to hold a view across a mutation that invalidates it.

`Scene::cubes()` returns a `std::span<const CubeInstance>` that is invalidated by any subsequent call to `update()` or `set_controller_poses()` (both call `cubes_.resize()` which may reallocate the vector). Nothing in the API or header comments warns callers of this. In `app.cpp` the span is passed directly to `render_eyes` in the same expression, so there is no current bug, but the contract is fragile. At minimum, add a header comment `/// Returned span is invalidated by update() or set_controller_poses().`; ideally, document whether the design intent is to keep this span-based API or change the call convention.

## Acceptance
- The header comment for `cubes()` explicitly states the invalidation condition.
- Alternatively, the return type is changed to something that makes the lifetime contract clear (e.g., a const reference to the vector, or a documented `[[nodiscard]]` annotation).
- No silent dangling-span usage exists in any call site.
