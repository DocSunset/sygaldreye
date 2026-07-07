# Prevent draw() on uninitialized CubeMesh

**File:** `components/cube_mesh/cube_mesh.hpp`
**Principle:** Objects must not be constructible into an unusable state; calling operational methods on an uninitialized instance must be impossible or immediately detectable.

`CubeMesh` is default-constructible with `vao_ = 0` and `vbo_ = 0`. Calling `draw()` on an un-`init()`ed instance binds VAO 0 and draws with invalid GL state, producing undefined rendering without any error. Add an assertion or debug check in `draw()` that `vao_ != 0`, or refactor `init()` into a factory that returns `std::optional<CubeMesh>` so callers cannot obtain an instance that is not ready to draw.

## Acceptance
- Calling `draw()` on an un-initialized `CubeMesh` either fails fast (assertion/abort) or is made impossible by the type system.
- If a factory is used, the default constructor is `private` or `= delete`.
- Build succeeds with no new warnings; existing usage in `renderer.cpp` is updated accordingly.
