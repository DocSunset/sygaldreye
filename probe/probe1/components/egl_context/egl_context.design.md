# egl_context

## Owning package
`render`

## Allowed dependencies
- EGL (system library)
- OpenXR (for `XrGraphicsBindingOpenGLESAndroidKHR`)
- Android log

## Ports

### Outputs
- `init()`: establishes an OpenGL ES 3.x context via a 1×1 pbuffer surface and returns a populated `RendererBinding` for use with `xrCreateSession`.

### Temporal couplings
- `init()` must complete before `xrCreateSession` is called (the binding is required by the XR runtime).

## Requirements
- Create an EGL display, choose an ES3-capable config, create an ES3 context, create a 1×1 pbuffer surface, and call `eglMakeCurrent`.
- Log `GL_VERSION`, `GL_RENDERER`, `GL_VENDOR` on success.
- Log and return `std::nullopt` on any EGL failure.
- RAII: destructor tears down surface, context, and display in reverse order.
