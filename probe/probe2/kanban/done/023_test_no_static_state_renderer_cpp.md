# Move static locals in render_eyes to instance members

**File:** `components/renderer/renderer.cpp`
**Principle:** Static local variables for mutable state persist across test runs, making test isolation impossible without a process restart; use instance members instead.

`render_eyes()` contains three static local variables: `static bool first` (one-shot log), `static double lastLocateErr` (rate-limit for locate errors), and `static bool layerLogged` (one-shot log). Because statics persist for the lifetime of the process, any test that exercises `render_eyes` more than once in a test suite sees incorrect behavior on the second run — the first-time log never fires again, and the rate-limit state carries over. Moving these to `Renderer` member fields allows a test fixture to reset them by constructing a fresh `Renderer`.

## Acceptance
- `first`, `lastLocateErr`, and `layerLogged` are `Renderer` member fields (initialized in the struct definition or constructor), not `static` locals
- `render_eyes` reads and writes `this->` versions of these fields
- No `static` keyword remains on mutable locals in `render_eyes`
