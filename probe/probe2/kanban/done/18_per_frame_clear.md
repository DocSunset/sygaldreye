# 18 — per-frame animated clear

Clear each eye to a time-varying color.

## Component
`render` / `renderer`

## Scope
- Per view per frame: `xrAcquireSwapchainImage`/`xrWaitSwapchainImage` → bind
  FBO → `glClearColor(f(predictedDisplayTime))` → `glClear` →
  `xrReleaseSwapchainImage`.

## Depends on
- 17

## Acceptance
- No GL/XR errors; color value advances each frame (logged).
