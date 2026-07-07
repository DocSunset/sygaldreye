# Remove jni.h, EGL, GLES3, and openxr_platform.h from xr_session.hpp

**File:** `components/xr_session/xr_session.hpp`
**Principle:** Headers should include only what is needed to express the public API; implementation-only includes belong in .cpp files.

`xr_session.hpp` opens with `#define XR_USE_PLATFORM_ANDROID` / `XR_USE_GRAPHICS_API_OPENGL_ES` and includes `jni.h`, `EGL/egl.h`, `GLES3/gl3.h`, and `openxr/openxr_platform.h` (lines 1–10). None of these types appear in `XrSessionObj`'s public API — the only public parameter that touches them is the `XrGraphicsBindingOpenGLESAndroidKHR&` in `create()`. Every consumer of `xr_session.hpp` silently acquires Android NDK and GL headers.

## Acceptance
- `jni.h`, `EGL/egl.h`, `GLES3/gl3.h`, and `openxr/openxr_platform.h` are removed from `xr_session.hpp`.
- The `XR_USE_PLATFORM_ANDROID` and `XR_USE_GRAPHICS_API_OPENGL_ES` defines move to `xr_session.cpp`.
- If `create()` keeps its `XrGraphicsBindingOpenGLESAndroidKHR` parameter, a forward declaration or a thinner type is used, or the parameter type changes to `const void*` with a cast inside the `.cpp`.
- The build still succeeds.
