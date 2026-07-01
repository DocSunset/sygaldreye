# Replace static booleans in Renderer::render_eyes with instance members

**File:** `components/renderer/renderer.cpp`
**Principle:** Mutable static variables introduce hidden global state and thread-safety hazards; replace with instance state or thread-safe one-time primitives.

`Renderer::render_eyes` uses three function-local statics: `static bool first`, `static double lastLocateErr`, and `static bool layerLogged`. These are shared across all `Renderer` instances and all calls, making the class non-reentrant and the statics effectively global. Move each to a corresponding `Renderer` member: `bool render_started_ = false`, `double last_locate_err_ = 0.0`, and `bool layer_logged_ = false`. This keeps the same throttling behavior while scoping it correctly to each instance.

## Acceptance
- All three statics in `render_eyes` are replaced by instance member variables.
- `Renderer`'s header declares the three new members (initialized to their defaults).
- Build succeeds with no new warnings.
