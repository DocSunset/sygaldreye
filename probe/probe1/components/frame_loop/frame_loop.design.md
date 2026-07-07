# frame_loop

Owning package: `xr`

## Ports

### Sources (coupled inputs)
- `XrSession` handle passed per call to `run_frame`

### Inputs
- `on_render` callback: invoked with the predicted display time when `shouldRender` is true; returns `FrameLayers` to submit

### Outputs
- Drives xrWaitFrame / xrBeginFrame / xrEndFrame each call
- Logs frame-drop counts every 100 frames and a heartbeat every 5 seconds
- Logs a warning when the render callback exceeds 9 ms

### Temporal couplings
- `run_frame` must only be called while the XrSession is running (after xrBeginSession, before xrEndSession)
- Per-frame ordering is fixed: xrWaitFrame → xrBeginFrame → on_render → xrEndFrame; must not be interleaved

### Intended seams
- `on_render` callback: caller provides projection layers; defaults to zero layers if not set or returns empty

## Requirements
- Execute the xrWaitFrame / xrBeginFrame / xrEndFrame cycle each call
- Time the render callback and warn if it exceeds the frame budget
- Track and periodically log frame drops (xrEndFrame failures)
- Log a heartbeat every 5 seconds to confirm the loop is alive

## Allowed dependencies
- OpenXR loader
- Android log
