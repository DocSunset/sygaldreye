# Replace XR_INFINITE_DURATION swapchain wait with finite timeout and error check

**File:** `components/renderer/renderer.cpp`
**Principle:** Use finite timeouts and check return codes; never block indefinitely or silently proceed after failure.

In `render_eyes()` (lines 196-198), `xrWaitSwapchainImage()` is called with `XR_INFINITE_DURATION`, which can hang the app indefinitely if the XR runtime stalls. Additionally, `xrAcquireSwapchainImage()` result is only logged via `XR_LOG_ERR` (not checked), so if it fails `index` is uninitialized and the subsequent `e.fbo(index)` dereferences garbage memory.

Set `wi.timeout` to a finite value (e.g., `5,000,000` nanoseconds = 5 ms). Check the result of `xrAcquireSwapchainImage`; if it fails, skip rendering that eye and return false. Check the result of `xrWaitSwapchainImage`; if it times out or fails, release any acquired image and skip the eye.

## Acceptance
- `wi.timeout` is set to a finite value, not `XR_INFINITE_DURATION`.
- `xrAcquireSwapchainImage` result is checked; failure skips the eye without using `index`.
- `xrWaitSwapchainImage` result is checked; timeout/failure skips the eye and logs a warning.
- App cannot hang indefinitely in `render_eyes()`.
