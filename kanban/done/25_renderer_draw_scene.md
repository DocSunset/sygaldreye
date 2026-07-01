# 25 — renderer draws scene cubes (completes backlog 01)

Wire the pieces: per eye, build view+projection from the located pose/fov and draw each scene cube. Replaces the animated color clear with a 3D scene.

## Component
- Modify `renderer` (package `render`); `app` drives scene.update + passes cubes.

## Ports / seam
- `renderer` gains a draw seam: `render_eyes(..., std::span<const CubeInstance> cubes)` (or a draw callback). Renderer owns view/proj; scene owns models.
- Per eye: `mvp = projection(fov) * view(pose) * cube.model` via `vr_math`; call `cube_mesh::draw(mvp)`.
- `app` (platform): each frame call `scene.update(now)`, pass `scene.cubes()` into the render seam.

## Scope
- Clear to a dark background (not animated). Draw cubes with depth test.
- Remove the per-frame sin-color clear logging once cube renders.

## Depends on
- 20 (vr_math), 21 (depth), 23 (cube_mesh), 24 (scene)

## Acceptance — backlog 01
- A floating cube with distinctly colored faces is visible, rotating, on Quest 3.

## Review checkpoint (items 25 + 26)

**Branch:** `items/20-28-vr-scene`

**Commands:**
```bash
git checkout items/20-28-vr-scene
sh/build.sh
sh/package.sh
adb uninstall com.eyeballs.vr || true
adb install -r build/apk/eyeballs.apk
adb logcat -c
adb shell monkey -p com.eyeballs.vr -c android.intent.category.LAUNCHER 1
# Put on headset, wait ~8 seconds
adb logcat -d -s eyeballs:V
```

**What to look for:**
- A dark background with a single spinning cube ~2m in front of you at roughly eye height
- Cube has 6 distinctly colored faces: red, cyan, green, magenta, blue, yellow
- Cube rotates smoothly (~1 rad/s around Y)
- Item 26 (fixed in space): move your head left/right/up/down — the cube should stay anchored to the world, not follow your head
- Stereoscopic depth should look correct (solid 3D object, not flat)
