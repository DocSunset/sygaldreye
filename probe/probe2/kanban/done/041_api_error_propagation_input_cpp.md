# Propagate xrSyncActions and xrLocateSpace errors from Input::sync

**File:** `components/input/input.cpp`
**Principle:** Errors must propagate to callers with sufficient context; void functions that silently swallow failures leave the program in an indeterminate state.

`Input::sync` calls `xrSyncActions` (line ~83) without checking its return value — if the action set is not attached or the session is invalid, the call fails silently and subsequent `xrLocateSpace` calls return invalid locations. `xrLocateSpace` is also called without checking its return value; only the `locationFlags` field is inspected, so an outright API failure is ignored. Change `sync` to return `XrResult`, check both calls, and return the first error to the caller so it can log and skip the frame.

## Acceptance
- `Input::sync` returns `XrResult` instead of `void`.
- The return value of `xrSyncActions` is checked; on failure the function returns that result immediately.
- The return value of `xrLocateSpace` is checked; on failure the corresponding pose is marked invalid and the result is returned.
- The call site in `app.cpp` checks the returned result and handles it (e.g., logs and continues).
- Build succeeds with no new warnings.
