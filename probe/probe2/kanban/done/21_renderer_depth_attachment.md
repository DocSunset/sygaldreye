# 21 — per-eye depth attachment

3D geometry needs a depth buffer. Today each eye FBO has color only.

## Component
- Modify `renderer` (package `render`).

## Scope
- Add a depth renderbuffer (`GL_DEPTH_COMPONENT24`) per eye FBO, sized to swapchain dims; attach as `GL_DEPTH_ATTACHMENT`.
- Keep `GL_FRAMEBUFFER_COMPLETE` check.
- In the per-frame eye pass: `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`, enable `GL_DEPTH_TEST`.
- Update `EyeSwapchain` to hold depth renderbuffer handle(s).

## Depends on
- nothing

## Acceptance
- App still runs on Quest 3, framebuffers report complete, no GL errors logged.
