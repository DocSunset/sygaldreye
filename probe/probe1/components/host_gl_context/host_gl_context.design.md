# host_gl_context

Headless OpenGL ES 3 context for the Linux host (Mesa/EGL), used as a prerequisite for host-side rendering tests and shader validation.

## Ports

- **Inputs**: none
- **Outputs**: none
- **Sources**: EGL default display (Mesa platform)
- **Destinations**: none
- **Temporal couplings**: `create()` must be called before any GL calls; destructor must be called before the process exits to release EGL resources
- **Intended seams**: `HostGlContext` can be created and passed into any host-side GL test fixture

## Requirements

- `create()` returns `std::nullopt` on any EGL failure rather than throwing
- Move-only RAII; destructor releases the EGL surface, context, and display in that order
- Uses a 1×1 pbuffer surface; rendering targets an FBO, not the surface
- No Android, JNI, XR, or NDK headers

## Allowed dependencies

- EGL (`libEGL`)
- OpenGL ES 3 (`libGLESv2`)
- C++ standard library

## Owning package

`render`
