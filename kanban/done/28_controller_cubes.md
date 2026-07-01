# 28 — cubes attached to controllers (completes backlog 03)

Feed controller poses into `scene` so a cube is drawn at each hand, reusing `cube_mesh`.

## Component
- Wire `input` → `scene` → `renderer` in `app`; small `scene` addition.

## Scope
- Each frame: `input.sync(time)`; pass valid hand poses to `scene.set_controller_poses(left, right)`.
- `scene.cubes()` appends one (smaller) cube per valid controller pose, model = `vr_math::pose_to_world(handPose) * scale`.
- No new geometry — reuse `cube_mesh`.

## Depends on
- 25 (scene→render draw path), 27 (input)

## Acceptance — backlog 03
- A cube tracks each controller, moving/rotating with it; the floating cube (01/02) still renders.
- Verify on-device with headset worn and controllers in hand.

## Review checkpoint (item 28)

**Branch:** `items/20-28-vr-scene` (same as items 25/26 — already installed if you reviewed those)

**Commands (if re-installing):**
```bash
git checkout items/20-28-vr-scene
sh/build.sh && sh/package.sh
adb uninstall com.eyeballs.vr || true
adb install -r build/apk/eyeballs.apk
adb logcat -c
adb shell monkey -p com.eyeballs.vr -c android.intent.category.LAUNCHER 1
# Put on headset, pick up controllers
sleep 8
adb logcat -d -s eyeballs:V
```

**What to look for:**
- The floating cube is still visible, rotating, world-anchored (backlog 01/02 still works)
- A small cube (~10cm) appears at each controller's grip position
- Controller cubes move and rotate in real time as you move your hands
- Cubes disappear if controller tracking is lost, reappear when re-tracked
- No crashes or GL errors in logcat
