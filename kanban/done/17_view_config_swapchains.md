# 17 — view config & swapchains

Create per-eye swapchains and framebuffers.

## Component
`render` / `renderer`

## Scope
- Enumerate `PRIMARY_STEREO` view config + views.
- Create a color `XrSwapchain` per view (GLES format); make GL framebuffers
  per swapchain image.

## Depends on
- 16

## Acceptance
- Swapchains + FBOs created for both eyes; no GL/XR errors.
