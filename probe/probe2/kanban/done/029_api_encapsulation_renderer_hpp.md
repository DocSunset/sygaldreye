# Make Renderer and EyeSwapchain implementation details private

**File:** `components/renderer/renderer.hpp`
**Principle:** Implementation details must be private; only the minimal necessary interface is public, so callers cannot depend on internal layout or accidentally mutate internal state.

`Renderer` exposes all EGL handles (`display`, `config`, `context`, `surface`), the full `EyeSwapchain` array, and the cached `projViews`/`projLayer` output as public members. `EyeSwapchain` likewise exposes `images`, `fbos`, and `depth_rbs` publicly. External code (e.g., `app.cpp`) references `state.renderer.projLayer` directly by address to build the layer list, coupling it to the internal struct layout. Move all data members in both `Renderer` and `EyeSwapchain` to `private`, add accessors for `width`, `height`, `swapchain` handle, and expose the projection layer via a method (e.g., `const XrCompositionLayerProjection* last_proj_layer() const`) rather than by direct member access.

## Acceptance
- All data members of `EyeSwapchain` are `private`; public interface is limited to `width()`, `height()`, `fbo(index)`, and `swapchain()`.
- All data members of `Renderer` are `private`; EGL handles are not accessible from outside.
- `app.cpp` accesses the projection layer through a method, not `&state.renderer.projLayer`.
- Build succeeds with no new warnings.
