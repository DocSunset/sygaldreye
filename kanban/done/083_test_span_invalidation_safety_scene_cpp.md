# Fix span invalidation: Scene::cubes() returns a span into a resizable vector

**File:** `components/scene/scene.cpp`
**Principle:** Do not return spans or pointers into containers that may be resized; the span is silently invalidated, producing undefined behavior in callers that retain it across a resize.

`Scene::cubes()` returns `std::span<const CubeInstance>` over the internal `cubes_` vector. Both `update()` and `set_controller_poses()` call `cubes_.resize(1)`, which may reallocate and invalidate any outstanding span. In `app.cpp`, `render_eyes` is called with the span returned by `cubes()` immediately after `set_controller_poses()`, so in practice no span is retained across a resize — but the API contract does not enforce this, and a future caller could easily violate it. The fix is to document the lifetime contract explicitly in the header (span is valid only until the next mutating call) or to change the API so callers cannot hold a stale span (e.g., pass a callback or return by value for small collections).

## Acceptance
- The header for `Scene::cubes()` includes a doc comment stating the span is valid only until the next call to `update()` or `set_controller_poses()`
- OR the API is changed to eliminate the dangling-span risk (e.g., the render callback receives the span directly, or the method takes a visitor)
- A test exists that calls `update()` or `set_controller_poses()` and then `cubes()` in the correct order, verifying the count
