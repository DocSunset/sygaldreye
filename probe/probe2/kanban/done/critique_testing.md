# Testing Coverage and Testability Audit

## What the Current Tests Actually Verify

**vr_math.test.cpp (86 lines, 7 tests)**
- Identity pose → identity matrix; 45° symmetric FOV projection; 90° Y-axis quaternion conversion
- Tests are mathematically sound and use `isApprox()` with 1e-5 tolerance
- Missing: asymmetric FOV, near-zero angles, extreme angles (120°/eye), head position + rotation combined, coordinate handedness

**scene.test.cpp (27 lines, 3 tests)**
- Single cube created on first `update()`; model matrix changes over time; cube at (0, 1.5, -5)
- Missing: `set_controller_poses()` is entirely untested; repeated-update semantics; resize side effects

**hello.test.cpp (9 lines, 1 test)**
- Writes "hello, world\n" to an ostringstream and verifies exact match
- Only host-side test; runs without a device

**renderer.test.cpp (20 lines, 1 test)**
- Calls `Renderer::init()` on-device and checks GL_VERSION/GL_RENDERER/GL_VENDOR are non-null
- Sanity check, not a functional test; no rendering logic exercised

---

## Critical Coverage Gaps

### Renderer (renderer.cpp, 239 lines) — Almost Entirely Untested

`create_swapchains()` is 80+ lines mixing view enumeration, format negotiation, and FBO/RBO creation. Zero tests. Wrong format choice or FBO incompleteness fail silently. The policy (format selection) and mechanism (GL object creation) are fused, preventing independent testing.

`render_eyes()` is 80+ lines of eye loop, view location, swapchain acquire/release, MVP computation, and draw submission. Zero tests. Static variables `lastLocateErr`, `first`, and `layerLogged` make test isolation impossible — there is no way to reset them between test runs.

### Input (input.cpp, 105 lines) — Zero Tests

`create()` sets up XR action sets, binding suggestions, and hand spaces. `sync()` acquires hand pose tracking. A static `logged` flag prevents verifying logging behavior twice. Both functions require an active XR instance and session, so they can only run on-device; but even there, no tests exist.

### XrSessionObj (xr_session.cpp, 152 lines) — Zero Tests

The state machine in `poll_events()` handles 9 session states (READY→BEGIN, STOPPING→END, EXITING/LOSS_PENDING→quit). No tests. State transition bugs — multiple changes in one poll, exiting before ready, loss during running — go undetected.

`render_frame()` contains frame skipping logic (`shouldRender && on_render`) and the layer submission protocol. Bugs in the `shouldRender` branch or in the interaction between frame prediction and layer submission are invisible.

### App Integration (app.cpp, 98 lines) — Zero Tests

Hand cube matrix chain (`pose_to_world → local_T → scale`, lines 66-82) and controller offset constants are computed inline with no coverage. The ordering constraint between `update()` and `set_controller_poses()` is never validated.

### CubeMesh (cube_mesh.cpp, 103 lines) — Zero Tests

Vertex stride (24 bytes), attribute offsets (0 vs 12), and format (GL_FLOAT) are untested. The MVP uniform binding in `draw()` — including the Eigen column-major layout assumption — is not exercised.

### GlProgram (gl_program.cpp, 62 lines) — Zero Tests

Shader compilation error handling, program link errors, and uniform location failures all silently log but are never verified.

---

## Design Choices That Impede Testability

**Direct OpenXR/EGL calls without abstraction.** All XR and graphics setup is inline in the implementations. No seam exists for offline testing with stubs. On-device execution is the only viable test path for most components.

**Static variables for mutable state.** `render_eyes()` has `first`, `lastLocateErr`, `lastEndErr`; `render_frame()` has `lastHeartbeat`, `firstFrame`; `input::sync()` has `logged`. Statics persist across test runs; test isolation requires a process restart.

**Monolithic functions mixing policy and mechanism.** `create_swapchains()` enumerates views, selects format, and creates GL objects in a single 80-line function. The format-selection policy cannot be tested without allocating real GL objects.

**No error returns — only logging.** Most error conditions call `__android_log_print()` and continue or return bool. Tests cannot capture or verify that the right error was emitted, and cannot distinguish "not supported" from "system error."

**`scene::cubes()` returns a span that may be invalidated.** The vector is resized on `update()` and `set_controller_poses()`. Callers holding a prior span observe undefined behavior.

---

## Bugs the Current Tests Cannot Catch

1. **Matrix layout:** Eigen is column-major; no test verifies that GLSL receives the correct layout.
2. **Coordinate frame mismatches:** OpenXR Y-up vs scene assumptions; hand pose to world space never exercised.
3. **Swapchain format fallback:** The SRGB8 → RGBA8 → first-available fallback is never tested; a device without SRGB fails silently.
4. **Eye pose validity:** `viewStateFlags` check is implemented but never exercised with invalid poses.
5. **Frame skip logic:** `shouldRender == false` path in the frame loop is never covered.
6. **Input action binding failures:** If the Oculus Touch interaction profile is unavailable, the app continues silently.
7. **Scene state corruption:** Calling `update()` after `set_controller_poses()` silently erases hand cubes via `resize(1)`. Not caught.
8. **GL error accumulation:** No `glGetError()` calls in the render loop; errors accumulate invisibly.
9. **FOV edge cases:** Angles ≥ 90° or extreme asymmetric values not tested.

---

## Testability Table

| Component   | LOC | Test LOC | Can Test Offline | Issues |
|-------------|-----|----------|-----------------|--------|
| hello       | 7   | 9        | Yes             | Covered |
| vr_math     | 49  | 86       | Yes             | Edge cases missing |
| scene       | 30  | 27       | Yes             | `set_controller_poses` untested |
| renderer    | 239 | 20       | No              | Needs EGL + XR; static state |
| input       | 105 | 0        | No              | Needs XR instance/session |
| xr_session  | 152 | 0        | No              | Needs OpenXR runtime |
| app         | 98  | 0        | No              | Needs NativeActivity |
| cube_mesh   | 103 | 0        | No              | Needs GL context |
| gl_program  | 62  | 0        | No              | Needs GL context |

---

## Concrete Improvements

### Priority 1: Tests That Should Exist Now

Add `set_controller_poses()` coverage to `scene.test.cpp`: both hands valid, left only, right only, neither, multiple calls, verify cube count (1 + valid hands) and update ordering invariant.

Add edge cases to `vr_math.test.cpp`: asymmetric FOV, near-zero angles, extreme angles, combined position + rotation.

Extend `renderer.test.cpp`: verify `glGetString()` works post-init, check extension strings.

### Priority 2: Structural Refactoring for Testability

Extract `choose_swapchain_format(std::vector<int64_t> formats) -> int64_t` from `create_swapchains()`. Pure function, testable offline, covers all fallback paths.

Split MVP computation out of `render_eyes()` into a free function — testable without a live session.

Move static variables in `render_eyes()`, `render_frame()`, and `sync()` into instance members so test fixtures can reset them.

Add `glCheck()` wrapper calling `glGetError()` after each GL call, logging with file/line context.

Return error codes instead of bools where the distinction between "not available this frame" and "system error" matters (especially `input::sync()`).

### Priority 3: Integration Tests (On-Device)

`renderer_integration.test.cpp`: test `create_swapchains()` return value and FBO completeness. Test `render_eyes()` with a stub scene (zero cubes). Verify `projLayer` has correct eye count.

State machine test: write a test fixture that replays XR event sequences through `poll_events()` and asserts the resulting session state.

### Priority 4: Gaps That Must Remain On-Device

EGL context creation, OpenXR session lifecycle, swapchain/FBO creation, input action/binding setup, GL shader compilation. These require hardware but should have at least smoke tests confirming they complete without error.
