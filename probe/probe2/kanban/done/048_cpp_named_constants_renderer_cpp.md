# Replace magic near/far plane literals in renderer with named constants

**File:** `components/renderer/renderer.cpp`
**Principle:** Magic numbers embedded in expressions must be replaced with named `constexpr` constants so that the intent is self-documenting and adjustments require changing only one definition.

`Renderer::render_eyes()` calls `projection(views[eye].fov, 0.05f, 100.0f)` with hard-coded near and far plane values. These values control the depth range for both eyes on every frame but are unnamed, making their role unclear and forcing a search-and-replace if the clipping distances need tuning. Define `constexpr float kNearPlane = 0.05f;` and `constexpr float kFarPlane = 100.0f;` at file scope and use them in the call.

## Acceptance
- `kNearPlane` and `kFarPlane` are defined as `constexpr float` at file scope in `renderer.cpp`
- The `projection(...)` call uses these constants instead of literals
- No behaviour change; build succeeds with no new warnings
