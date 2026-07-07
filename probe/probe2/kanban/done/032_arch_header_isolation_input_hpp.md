# Remove jni.h, EGL, and GLES3 from input.hpp

**File:** `components/input/input.hpp`
**Principle:** Headers should include only what is needed to express the public API; implementation-only includes belong in .cpp files.

`input.hpp` opens with `#define XR_USE_PLATFORM_ANDROID` / `XR_USE_GRAPHICS_API_OPENGL_ES` and includes `jni.h`, `EGL/egl.h`, and `GLES3/gl3.h` before `openxr.h`. The `Input` struct's public API uses only `XrInstance`, `XrSession`, `XrSpace`, `XrTime`, and `XrPosef` — none of which require the platform-specific EGL or GLES defines. Every consumer of `input.hpp` silently acquires Android NDK and GL headers.

## Acceptance
- `jni.h`, `EGL/egl.h`, `GLES3/gl3.h`, and the `XR_USE_GRAPHICS_API_OPENGL_ES` define are removed from `input.hpp`.
- `XR_USE_PLATFORM_ANDROID` and `XR_USE_GRAPHICS_API_OPENGL_ES` defines (if needed) move to `input.cpp`.
- The build still succeeds.
