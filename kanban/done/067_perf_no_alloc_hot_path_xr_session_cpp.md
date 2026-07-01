# Eliminate per-frame std::vector allocation in render_frame

**File:** `components/xr_session/xr_session.cpp`
**Principle:** Never allocate or resize containers on the per-frame hot path; preallocate everything at init time.

`XrSessionObj::render_frame()` constructs a `std::vector<const XrCompositionLayerBaseHeader*>` on the heap every frame (line 639: `std::vector<const XrCompositionLayerBaseHeader*> layers;`) and the `on_render` callback also returns a `std::vector` by value, causing an additional allocation per frame. At 90 Hz this is 180+ heap allocations per second in the critical frame path.

Replace `layers` with a fixed-size `std::array` or a small stack buffer (max layer count is small and bounded by `XrSystemGraphicsProperties::maxLayerCount`, typically 16). Change `on_render`'s return type to a span or a small fixed-size struct. Alternatively, pass an output parameter by reference.

## Acceptance
- No heap allocation occurs inside `render_frame()` on the hot path (verified by inspection or by adding an allocator hook in tests).
- The `on_render` callback signature no longer returns `std::vector` by value.
- Frame submission behavior is unchanged.
