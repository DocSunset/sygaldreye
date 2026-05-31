# renderer

## Owning package
`render`

## Allowed dependencies
- platform (for Android log)
- EGL, OpenGL ES 3.x (system libraries)

## Ports

### Outputs
- A current OpenGL ES 3.x context (via `eglMakeCurrent`) available to subsequent GL calls on the same thread.
- The `EGLDisplay`, `EGLConfig`, `EGLContext`, and `EGLSurface` handles (members of `Renderer`) for use by the XR graphics binding (future work).

### Temporal couplings
- `init()` must be called after `xrGetSystem` succeeds (system availability is required before the XR graphics binding is attached, but the EGL context itself does not yet depend on XR).
- The XR OpenGL ES graphics binding (`XR_KHR_opengl_es_enable`) will consume the EGL handles in a later work item; that binding must be registered before `xrCreateSession`.

## Requirements
- Bring up an ES 3.x context using a 1×1 pbuffer surface (XR owns real framebuffers).
- Log `GL_VERSION`, `GL_RENDERER`, `GL_VENDOR` on success.
- Return `false` and log `eglGetError` on any EGL failure.

## Notes
- No window surface is created here; XR swapchain surfaces are managed by the `render` package in later items.
