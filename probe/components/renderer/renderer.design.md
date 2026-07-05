# renderer

## Owning package
`render`

## Allowed dependencies
- platform (for Android log)
- xr (reads XrInstance, XrSystemId, XrSession to create swapchains)
- EGL, OpenGL ES 3.x (system libraries)

## Ports

### Inputs
- `create_swapchains(XrInstance, XrSystemId, XrSession)`: called once after session creation; enumerates stereo views and creates one XrSwapchain + GL FBOs per eye.

### Outputs
- A current OpenGL ES 3.x context (via `eglMakeCurrent`) available to subsequent GL calls on the same thread.
- `graphics_binding()`: returns a populated `XrGraphicsBindingOpenGLESAndroidKHR`.
- `eyes[2]` (`EyeSwapchain`): per-eye swapchain handle, width/height, swapchain images, and FBO array. Accessor: `eyes[i].fbo(imageIndex)` returns the GL framebuffer for acquired image `imageIndex`.

### Temporal couplings
- `init()` must be called before `xrCreateSession` (EGL context needed for graphics binding).
- `create_swapchains()` must be called after `xrCreateSession` and while the EGL context is current.

## Requirements
- Bring up an ES 3.x context using a 1×1 pbuffer surface (XR owns real framebuffers).
- Log `GL_VERSION`, `GL_RENDERER`, `GL_VENDOR` on init.
- Enumerate `XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO`; expect 2 views. Log view count and recommended dimensions.
- Choose swapchain format: prefer `GL_SRGB8_ALPHA8`, fallback `GL_RGBA8`, else first supported. Log chosen format.
- Create one `XrSwapchain` per eye (color attachment, recommended size, sampleCount 1).
- Create one GL framebuffer per swapchain image; check `GL_FRAMEBUFFER_COMPLETE` for first image per eye.
- Log and return `false` on any XR or EGL failure.
