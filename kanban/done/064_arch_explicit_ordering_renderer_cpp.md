# Make renderer initialization ordering impossible to violate

**File:** `components/renderer/renderer.cpp`
**Principle:** APIs must make required initialization ordering impossible to violate at the call-site, not merely conventional.

`Renderer::init()` initializes EGL state, after which `graphics_binding()` is valid and must be called before `xr_session.create()`, and `create_swapchains()` must be called only after the session exists. None of this is enforced by the type system — calling `graphics_binding()` on a default-constructed `Renderer` silently returns garbage `EGLDisplay`/`EGLContext` handles. The three-step init sequence in `app.cpp` (lines 45–47) is only conventional.

## Acceptance
- `init()` returns a capabilities/binding struct (e.g. `RendererBinding`) that is the only way to get a `XrGraphicsBindingOpenGLESAndroidKHR`; `graphics_binding()` is removed as a separate method.
- `create_swapchains` requires the session handle to be passed in (already done), but is gated on a valid binding having been produced first (e.g. takes the binding struct).
- Calling in the wrong order produces a compile error or an explicit runtime assertion, not silent garbage.
- The build succeeds.
