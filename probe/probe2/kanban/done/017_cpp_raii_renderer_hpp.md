# Add destructors for EGL and swapchain resources in Renderer

**File:** `components/renderer/renderer.hpp`
**Principle:** Every resource acquired must have a deterministic release path — either a destructor or a smart wrapper — so that resources are freed when the owning object goes out of scope.

`Renderer` holds four EGL handles (`display`, `config`, `context`, `surface`) and `EyeSwapchain` holds `fbos`, `depth_rbs`, and `handle` (XrSwapchain), none of which are released anywhere. When either struct is destroyed, GPU and XR memory leaks. `Renderer` and `EyeSwapchain` both need destructors that call `eglDestroySurface`/`eglDestroyContext`/`eglTerminate` and `glDeleteFramebuffers`/`glDeleteRenderbuffers`/`xrDestroySwapchain` respectively, plus deleted copy constructors to prevent double-free.

## Acceptance
- `Renderer` has a destructor that releases `surface`, `context`, and `display` in reverse-acquisition order using sentinel checks (`EGL_NO_SURFACE`, etc.)
- `EyeSwapchain` has a destructor that calls `glDeleteFramebuffers`, `glDeleteRenderbuffers`, and `xrDestroySwapchain` when containers are non-empty / handle is valid
- Both types have deleted copy constructor and copy-assignment operator
- Build succeeds with no new warnings
