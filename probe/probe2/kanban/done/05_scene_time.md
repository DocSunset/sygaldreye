# scene time port

Thread elapsed time (seconds since session start) through the frame loop into the scene so that any component can animate against a monotonic clock. Prerequisite for all animation: wind sway, water ripples, particle motion, sky colour transitions.

## Approach

- The frame loop already calls the scene each frame. Extend the interface so it receives `float time_s` (seconds since XR session start, sourced from `XrTime` converted via `xrConvertTimeToWin32PerformanceCounterKHR` or simply accumulated frame durations).
- Add `float time_s` to whatever struct or parameter list the scene's per-frame update/draw entry point uses.
- No new component file required — this is a small interface change across `frame_loop`, `xr_session`, and `scene`.
- Update `scene.design.md` to document `time_s` as an input port.

## Acceptance

- `scene` receives a monotonically increasing `time_s` each frame
- `time_s` is plausible: roughly matches wall-clock seconds elapsed while the session runs
- No existing tests broken
