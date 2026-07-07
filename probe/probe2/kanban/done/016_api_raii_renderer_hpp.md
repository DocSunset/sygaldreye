# Add destructor and move semantics to Renderer and EyeSwapchain

**File:** `components/renderer/renderer.hpp`
**Principle:** Every resource acquired must have a deterministic release path; owning types must implement RAII (destructor + move, deleted copy).

`Renderer` acquires EGL resources (`eglCreateContext`, `eglCreatePbufferSurface`) and OpenXR swapchains (`xrCreateSwapchain`), and `EyeSwapchain` holds GL framebuffer and renderbuffer objects (`glGenFramebuffers`, `glGenRenderbuffers`) — none of which are ever freed. Both structs are copyable by default, risking double-free. Add destructors to `EyeSwapchain` (delete fbos, depth_rbs, destroy swapchain) and `Renderer` (destroy EGL context/surface/display), and delete copy operations while providing move operations for both.

## Acceptance
- `EyeSwapchain` destructor calls `glDeleteFramebuffers`, `glDeleteRenderbuffers`, and `xrDestroySwapchain` for non-zero/non-null handles.
- `Renderer` destructor calls `eglDestroyContext`, `eglDestroySurface`, and `eglTerminate` for non-null handles.
- Copy constructor and copy assignment are `= delete` on both types; move operations are defined.
- Build succeeds with no new warnings.
