# Extract per-frame orchestration from app.cpp into a frame_loop component

**File:** `components/app/app.cpp`
**Principle:** Each component should own one concern; mixing subsystem construction, hand-cube math, input sync, scene update, and render submission in one function creates an untestable god object.

`android_main` in `app.cpp` directly owns XrInstance, XrSystemId, Renderer, XrSessionObj, Scene, and Input (lines 43–48), then inlines the entire frame loop: input sync, hand transform computation with magic cm constants, scene update, render, and layer assembly (lines 57–97). Adding any new subsystem or frame-logic requires editing `android_main`. The hand cube offset math (lines 63–82) especially belongs in `scene` or a dedicated component, not the platform entry point.

## Acceptance
- A new `frame_loop` (or equivalent) component owns per-frame orchestration: input sync, hand transform computation, scene update, and render dispatch.
- `android_main` is reduced to initialization, running the loop, and teardown.
- Hand cube offset constants live in `scene` or the new component, not in `app.cpp`.
- The build succeeds and the app runs on-device with equivalent behavior.
