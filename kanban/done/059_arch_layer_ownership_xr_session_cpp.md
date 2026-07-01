# Decouple XR layer assembly from app.cpp via a LayerSubmitter abstraction

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Each layer should own its domain's logic; XR layer submission is an XR-session concern, not an app-orchestration concern.

`XrSessionObj::render_frame` accepts a callback returning `std::vector<const XrCompositionLayerBaseHeader*>` (xr_session.cpp line 115), and `app.cpp` (line 91) assembles the projection layer struct and casts it for submission. Adding a UI quad layer requires the app to know about XR composition layer types and memory lifetimes. The XR session should own layer assembly: it knows the projection layer geometry from `render_eyes`, and it should query an abstraction for additional layers rather than delegating raw pointer assembly to the caller.

## Acceptance
- `render_frame`'s callback no longer returns raw `XrCompositionLayerBaseHeader*` vectors assembled by the caller.
- Either: `render_frame` takes the renderer and calls it internally, or a `LayerSubmitter` interface is introduced that `xr_session` queries.
- `app.cpp` does not cast or construct `XrCompositionLayerProjection` structs.
- The build succeeds and the projection layer renders correctly on-device.
