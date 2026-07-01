# 26 — cube fixed in space (completes backlog 02)

Head tracking and binocular vision fall out of item 25: `renderer` already locates each eye's pose/fov and item 25 builds a per-eye view matrix. This item confirms correctness and pins the cube to world space.

## Component
- Verify/adjust `scene` + `renderer`; no new component expected.

## Scope
- Ensure the floating cube's model is expressed in the world reference space (STAGE/LOCAL), not relative to the head — independent of view matrix.
- Confirm both eyes use their own pose/fov (no shared/averaged matrix) so the cube has correct stereo parallax.
- If any head-locking bug surfaces, fix here.

## Depends on
- 25

## Acceptance — backlog 02
- Walking/turning the head keeps the cube anchored at a fixed location; it reads as solid 3D (correct depth) through both eyes.
- Run on-device with headset worn (see pause-before-verification convention).

## Review note
Covered by the same session as item 25 — see `25_renderer_draw_scene.md` for branch and commands.
The cube model is in world space by construction (scene sets a fixed world-space translation; renderer applies a per-eye view matrix). If the cube head-locks, the bug is in `vr_math::view()` or the per-eye pose not being used — check logcat `view[0] pos=` lines to confirm per-eye poses differ between frames as you move.
