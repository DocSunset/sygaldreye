# 11 — EGL context

Create a current GLES 3.2 context.

## Component
`render` / `renderer`

## Scope
- `eglGetDisplay`/`eglInitialize`/`eglChooseConfig` (ES3, no window)/
  `eglCreateContext` (v3)/pbuffer surface/`eglMakeCurrent`.
- Log `GL_VERSION`/`GL_RENDERER`. Start `renderer.design.md`.

## Depends on
- 10

## Acceptance
- `eglMakeCurrent` succeeds on-device; ES 3.x version logged.
