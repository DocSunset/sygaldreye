# Add frame timing instrumentation and frame drop detection to render_frame

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Instrument the frame loop to detect and report frame budget overruns; silent drops are unacceptable.

`XrSessionObj::render_frame()` (lines 623-660) measures nothing. `xrEndFrame` failures are throttle-logged at most once per 2 seconds, so repeated frame drops are invisible. There is no measurement of how long `on_render` takes relative to the ~11.1 ms frame budget at 90 Hz. The existing `now_sec()` helper is present but unused in the frame loop.

Wrap the `on_render` call with `now_sec()` timestamps. If the render duration exceeds a threshold (e.g., 9 ms = 80% of the 11.1 ms budget), log a warning with the elapsed time. Keep a rolling frame drop counter; log it periodically (e.g., every 100 frames or every 5 seconds).

## Acceptance
- `on_render` duration is measured every frame.
- A log warning is emitted when duration exceeds 9 ms (configurable constant).
- Frame drops (xrEndFrame failures) are counted; the count is logged at least once per 5 seconds if nonzero.
- No new heap allocation is introduced in the measurement path.
