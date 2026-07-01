# Propagate errors from XrSessionObj::poll_events

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Errors must propagate to callers with sufficient context; void functions that silently swallow failures leave the program in an indeterminate state.

`poll_events()` is `void` and calls `xrBeginSession` and `xrEndSession` inside the event loop, logging failures but not returning them to the caller. If `xrBeginSession` fails, `sessionRunning_` is not set, but the caller never learns that the session failed to start — it simply stops rendering without explanation. Change `poll_events` to return `XrResult` (or `bool`) and propagate the first failure encountered, allowing the caller in `app.cpp` to log a meaningful error and exit cleanly.

## Acceptance
- `poll_events` returns `bool` (or `XrResult`) indicating whether all state transitions succeeded.
- A failure in `xrBeginSession` or `xrEndSession` is returned to the caller.
- The call site in `app.cpp` checks the return and handles errors appropriately.
- Build succeeds with no new warnings.
