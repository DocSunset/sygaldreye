# Gate xrSyncActions behind session focus state check

**File:** `components/input/input.cpp`
**Principle:** Gate heavyweight XR synchronization calls behind validity/state checks; don't call unconditionally every frame.

`Input::sync()` (line 487) calls `xrSyncActions()` unconditionally on every frame. The OpenXR spec requires `xrSyncActions` to only be called when the session is in SYNCHRONIZED, VISIBLE, or FOCUSED state; calling it at other times (e.g., during IDLE or READY, or after LOST_FOCUS) is at best a no-op and at worst a serialization stall. The call site in `app.cpp` (line 725) is inside `session_running()` but not gated on `should_render()` or focus state.

Add a boolean parameter or a separate guard in `Input::sync()` (or at its call site in `app.cpp`) that skips the `xrSyncActions` call when the session is not in a focused/visible state. Alternatively, expose a `sync_if_focused(bool focused)` variant. This matches the pattern already used for rendering (`if (state.xrSession.should_render())`).

## Acceptance
- `xrSyncActions` is not called when the session state is IDLE, READY, or STOPPING.
- Input sync is skipped (or no-ops gracefully) when `AppState::hasFocus` is false.
- Hand pose validity is correctly cleared when sync is skipped.
