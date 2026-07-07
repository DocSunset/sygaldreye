# Move mutable statics in render_frame to XrSessionObj members

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Mutable `static` local variables in class methods violate encapsulation and introduce hidden shared state that is not reset when a new object is constructed, preventing safe re-use and creating data races if the method is ever called concurrently.

`XrSessionObj::render_frame()` uses four `static` local variables: `lastHeartbeat`, `firstFrame`, `lastLocateErr` (in renderer but a parallel pattern), and `lastEndErr`. These are logically per-session state but are shared across all instances and survive object destruction. They should be moved to non-static members of `XrSessionObj` (e.g., `double last_heartbeat_ = 0.0; bool first_frame_ = true; double last_end_err_ = 0.0;`) and initialized in the struct definition.

## Acceptance
- No `static` local variables remain in `render_frame()`
- The equivalent fields exist as non-static members of `XrSessionObj` with appropriate default values
- Behavior is unchanged: heartbeat logs every 5 s, first-frame log fires once per session object
- Build succeeds with no new warnings
