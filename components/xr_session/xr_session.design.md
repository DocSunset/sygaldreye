# xr_session

Owning package: `xr`

## Ports

### Sources (coupled inputs)
- `XrInstance` from `platform/app`
- `XrSystemId` from `platform/app`
- `XrGraphicsBindingOpenGLESAndroidKHR` injected from `render` at session creation (not a compile-time dependency)

### Outputs
- `XrSession` handle
- `XrSpace` world reference space (STAGE preferred, LOCAL fallback)
- `XrSessionState` current session state (updated by `poll_events`)
- `should_render()` — true when state is SYNCHRONIZED, VISIBLE, or FOCUSED
- `should_quit()` — true when EXITING, LOSS_PENDING, or instance loss pending

### Temporal couplings
- Session must be created after the XrInstance and XrSystemId exist
- Session must be created after the GLES context is current (EGL context made current by renderer)
- `xrBeginSession` is called only after XR_SESSION_STATE_READY; `xrEndSession` only after XR_SESSION_STATE_STOPPING
- `render_frame()` must only be called when `session_running()` is true (after xrBeginSession, before xrEndSession)
- Per-frame ordering is fixed: xrWaitFrame → xrBeginFrame → xrEndFrame; these must not be interleaved

### Intended seams
- **Layer submission (render seam):** `render_frame(std::span<const XrCompositionLayerBaseHeader* const>)` — caller passes projection layers to submit in xrEndFrame; defaults to empty (zero layers). The `render` package will fill this seam with a projection layer.

## Requirements
- Create an XrSession bound to the GLES context via XrGraphicsBindingOpenGLESAndroidKHR
- Log success or failure with XrResult code

## Allowed dependencies
- `platform` (for XrInstance/XrSystemId types via openxr headers)
- OpenXR loader
- NOT `render` (graphics binding is injected, not included)
