# xr_session

Owning package: `xr`

## Ports

### Sources (coupled inputs)
- `XrInstance` from `platform/app`
- `XrSystemId` from `platform/app`
- `XrGraphicsBindingOpenGLESAndroidKHR` injected from `render` at session creation (not a compile-time dependency)

### Outputs
- `XrSession` handle

### Temporal couplings
- Session must be created after the XrInstance and XrSystemId exist
- Session must be created after the GLES context is current (EGL context made current by renderer)
- Future: frame loop (xrWaitFrame/xrBeginFrame/xrEndFrame) must run within the session lifetime

### Intended seams
- Layer submission: the rendered projection layers are injected into xrEndFrame (future item)

## Requirements
- Create an XrSession bound to the GLES context via XrGraphicsBindingOpenGLESAndroidKHR
- Log success or failure with XrResult code

## Allowed dependencies
- `platform` (for XrInstance/XrSystemId types via openxr headers)
- OpenXR loader
- NOT `render` (graphics binding is injected, not included)
