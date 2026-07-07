# Document projLayer/projViews validity contract in Renderer

**File:** `components/renderer/renderer.hpp`
**Principle:** Spans and pointer-based views must express their validity window; callers must not be able to hold a pointer to output data across a call that overwrites it.

`render_eyes()` fills `projLayer` and `projViews` as member-variable side effects and returns `bool`; on `true`, the caller is expected to pass `&projLayer` to `xrEndFrame`. This contract is described only in an inline comment on `render_eyes` in the header. If the caller stores the pointer across multiple frames and `render_eyes` is not called (e.g., `shouldRender == false`), the pointer remains valid but now references stale data from the previous frame, which may cause incorrect rendering. Add explicit header documentation stating the validity window (`projLayer` is valid only until the next call to `render_eyes`), and consider returning a struct that bundles the layer pointer with its validity flag to make the contract unambiguous.

## Acceptance
- The header documents that `projLayer`/`projViews` are valid only between a successful `render_eyes` call and the next call.
- The call site in `app.cpp` does not store the layer pointer across frames.
- Build succeeds with no new warnings.
