# Assert session_running() precondition in render_frame

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Initialize-before-use and operational-state preconditions must be enforced by assertions or types, not only by documentation or caller convention.

`render_frame()` calls `xrWaitFrame` and `xrBeginFrame` regardless of session state. If called when the session is not running (e.g., between STOPPING and READY), `xrBeginFrame` returns `XR_ERROR_SESSION_NOT_RUNNING` which is logged and silently returns — but the behavior is subtle and the caller (`app.cpp`) already guards with `if (state.xrSession.session_running())`. Adding an `assert(sessionRunning_)` at the top of `render_frame` makes the precondition explicit and catches misuse in debug builds immediately, rather than relying on the OpenXR error path.

## Acceptance
- `render_frame` begins with `assert(sessionRunning_)` (or equivalent debug assertion).
- The assertion fires in debug builds if `render_frame` is called when the session is not running.
- The existing `app.cpp` guard (`if (session_running())`) satisfies the assertion in normal operation.
- Build succeeds with no new warnings; `<cassert>` is included.
