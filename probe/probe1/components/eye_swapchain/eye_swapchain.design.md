# eye_swapchain

## Owning package
`render`

## Allowed dependencies
- OpenXR (for XrSwapchain management)
- OpenGL ES 3.x (for FBO/renderbuffer creation)
- Android log

## Ports

### Inputs
- `create_swapchains(XrInstance, XrSystemId, XrSession, out)`: called once after session creation; enumerates stereo views and creates one XrSwapchain + GL FBOs per eye.

### Outputs
- `EyeSwapchain`: per-eye swapchain handle, width/height, swapchain images, and FBO array. Accessor `fbo(imageIndex)` returns the GL framebuffer for an acquired image index.
- `choose_swapchain_format(formats)`: pure function selecting the best GL format from the runtime's supported list.

### Temporal couplings
- `create_swapchains()` must be called after `xrCreateSession` and while an EGL context is current.
- `EyeSwapchain` destructor calls GL delete functions — the EGL context must still be current when `EyeSwapchain` is destroyed.

## Requirements
- Prefer `GL_SRGB8_ALPHA8`, fallback `GL_RGBA8`, else first supported format.
- Enumerate `XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO`; expect exactly 2 views.
- Create one `XrSwapchain` per eye at recommended dimensions, sampleCount 1.
- Create one GL framebuffer (color + depth renderbuffer) per swapchain image.
- Log framebuffer completeness for each FBO. Log and return false on XR or GL failures.
- RAII: `EyeSwapchain` destructor destroys FBOs, renderbuffers, and the XR swapchain.
