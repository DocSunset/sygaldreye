# Move XrSessionObj API contracts from design doc into header comments

**File:** `components/xr_session/xr_session.hpp`
**Principle:** API contracts (preconditions, postconditions, ordering requirements) must live in the header as inline documentation, not only in separate design documents.

`xr_session.design.md` specifies several contracts that are invisible to readers of the header: `render_frame()` must only be called when `session_running()` is true; `create()` must be called after `renderer.init()` while the EGL context is current; the `instance` parameter must outlive the `XrSessionObj`. None of these appear in `xr_session.hpp`. Add `///` doc comments to `create()` and `render_frame()` in the header that state the preconditions, valid call sequences, and lifetime requirements, so a developer reading only the header has the information needed to use the API correctly.

## Acceptance
- `create()` has a doc comment stating the EGL context must be current and that `instance` must outlive the object.
- `render_frame()` has a doc comment stating it must only be called when `session_running()` returns true.
- Comments are in the header file, not only in the design doc.
- No new code changes required; documentation only.
