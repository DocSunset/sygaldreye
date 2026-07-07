# Replace static variables in XrSessionObj::render_frame with instance members

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Mutable static variables introduce hidden global state and thread-safety hazards; replace with instance state or thread-safe one-time primitives.

`XrSessionObj::render_frame` uses three function-local statics: `static double lastHeartbeat`, `static bool firstFrame`, and `static double lastEndErr`. These are shared globally across all `XrSessionObj` instances and calls. Move each to instance members in `xr_session.hpp`: `double last_heartbeat_ = 0.0`, `bool first_frame_ = true`, and `double last_end_err_ = 0.0`, preserving their behavior while scoping them to the instance.

## Acceptance
- All three statics in `render_frame` are replaced by instance member variables declared in the header.
- Build succeeds with no new warnings.
