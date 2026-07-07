# Move static locals in render_frame to instance members

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Static local variables for mutable state persist across test runs, making test isolation impossible without a process restart; use instance members instead.

`render_frame()` contains three static local variables: `static double lastHeartbeat`, `static bool firstFrame`, and `static double lastEndErr`. These persist for the lifetime of the process, so a second `XrSessionObj` constructed in the same test process (or a second call to `render_frame` after a logical reset) will see stale state from the previous run. Moving them to `XrSessionObj` member fields restores proper isolation: each object has its own independent counters, and a test fixture can reset them by constructing a fresh `XrSessionObj`.

## Acceptance
- `lastHeartbeat`, `firstFrame`, and `lastEndErr` are `XrSessionObj` member fields initialized to their default values, not `static` locals
- `render_frame` reads and writes `this->` versions of these fields
- No `static` keyword remains on mutable locals in `render_frame`
