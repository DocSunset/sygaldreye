# Move mutable statics in render_eyes to Renderer members

**File:** `components/renderer/renderer.cpp`
**Principle:** Mutable `static` local variables in class methods violate encapsulation and introduce hidden shared state that is not reset when a new object is constructed, preventing safe re-use and creating data races if the method is ever called concurrently.

`Renderer::render_eyes()` uses three `static` local variables: `first` (fires a one-time log), `lastLocateErr` (rate-limits error logging), and `layerLogged` (fires a one-time log). All three are logically per-`Renderer` state but persist globally. Move them to non-static `Renderer` members (`bool first_render_ = true; double last_locate_err_ = 0.0; bool layer_logged_ = false;`) initialized in the struct definition.

## Acceptance
- No `static` local variables remain in `render_eyes()`
- Equivalent fields exist as non-static members of `Renderer` with appropriate default values
- Behavior is unchanged: one-time logs fire once per `Renderer` object, error rate-limiting resets per object
- Build succeeds with no new warnings
