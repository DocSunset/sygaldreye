# host_gl_context component

A headless OpenGL ES context for the development machine (Linux + Mesa), with no Android, JNI, or XR dependencies. Prerequisite for any host-side rendering: scene preview image generation, shader unit tests, geometry visualisation.

## Approach

- Uses EGL with a pbuffer surface (`EGL_PBUFFER_BIT`) so no display window is required.
- `host_gl_context.hpp` declares:
  ```cpp
  struct HostGlContext {
      static std::optional<HostGlContext> create();
      ~HostGlContext();
      // move-only
  private:
      EGLDisplay display = EGL_NO_DISPLAY;
      EGLContext context = EGL_NO_CONTEXT;
      EGLSurface surface = EGL_NO_SURFACE;
  };
  ```
- `create()` requests GLES 3.2, a 1×1 pbuffer surface (size irrelevant — rendering targets an FBO, not the surface), and makes the context current.
- No XR headers, no `jni.h`, no `native_app_glue`. Builds and runs on the host with Mesa.
- Pull `libGL`/EGL headers via Nix (`pkgs.mesa.dev` or `pkgs.libGL`).
- `host_gl_context.test.cpp`: create a context, call `glGetString(GL_VERSION)`, assert the result is non-null. Run on host (not on-device).

## Acceptance

- `create()` succeeds on the dev machine and returns a live GLES 3.2 context
- `glGetString(GL_VERSION)` returns a non-null string after creation
- Destructor releases EGL resources without error
- `host_gl_context.design.md` present; `.cpp` under 100 lines of substantive code
