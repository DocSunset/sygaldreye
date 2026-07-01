# 12 — graphics binding port

Expose the EGL context as an OpenXR graphics binding.

## Component
`render` / `renderer`

## Scope
- Assemble `XrGraphicsBindingOpenGLESAndroidKHR` (display, config, context).
- Expose it as an output port for `xr_session` (injected at session create).

## Depends on
- 11

## Acceptance
- Binding struct populated + retrievable; builds clean.
