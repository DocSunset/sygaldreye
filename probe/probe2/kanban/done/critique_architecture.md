# Architectural Critique

## 1. Circular and Implicit Coupling via Renderer's Public Headers

`renderer.hpp` (lines 12-13) publicly includes `scene.hpp` and `cube_mesh.hpp`. `renderer/CMakeLists.txt` lists `scene` and `cube_mesh` as PUBLIC dependencies, exposing them to all consumers. This violates the intended architecture: CLAUDE.md says `scene` sits above `render`, yet any component including `renderer.hpp` now transitively depends on both.

In `app.cpp` (line 5), `renderer.hpp` is included, dragging in `scene.hpp` transitively. This is why `app.cpp` can construct `Scene` directly (line 24) without an explicit include. If `scene` changes its public API, `app` breaks even though architecturally it should be decoupled.

**Fix:** Remove `scene.hpp` and `cube_mesh.hpp` from `renderer.hpp`. They are only used in `renderer.cpp`. Change `renderer/CMakeLists.txt` from `PUBLIC scene cube_mesh` to `PRIVATE scene cube_mesh`. Let `app.cpp` include them directly if needed.

---

## 2. Renderer Owns Game Content Rendering Logic

`renderer::render_eyes` (lines 158-238) iterates `std::span<const CubeInstance>` cubes (lines 209-212) and computes projection and view matrices (lines 207-208). This couples rendering tightly to the cube concept. Adding new geometry types requires modifying `renderer.cpp`, a layer that should be geometry-agnostic.

Meanwhile `scene::cubes()` returns a list of cube instances the caller must know how to render — the seam is in the wrong place.

**Fix:** Move the eye loop and MVP computation into a context the caller populates. Expose a `begin_eye`/`end_eye` API that yields a `RenderContext` (projection, view, eye index) and let the caller issue draw calls for whatever geometry it owns.

---

## 3. App Component is a God Object

`app.cpp` (lines 39-97) creates and owns XrInstance, XrSystemId, Renderer, XrSessionObj, Scene, and Input directly (lines 43-48), then manually orchestrates the frame loop: poll events, check running state, sync input, compute hand transforms with magic constants (lines 66-82), update scene, render frame. This is all inline with no delegation.

Adding any new subsystem requires editing `app.cpp`. Testing the frame loop logic in isolation is impossible. Hand cube offset tuning is buried in the wrong layer.

**Fix:** Introduce a `frame_loop` (or `game`) component that owns per-frame orchestration and takes the subsystems as dependencies. `android_main` reduces to init + loop + teardown.

---

## 4. XrSessionObj Header Pollution

`xr_session.hpp` (lines 1-10) includes `jni.h`, `EGL/egl.h`, `GLES3/gl3.h`, and `openxr_platform.h`. None of these types appear in the struct's public API — they are only needed in `xr_session.cpp`. Any component including `xr_session.hpp` silently acquires Android NDK and graphics headers as implicit dependencies.

**Fix:** Remove those includes from the header. Keep them in the `.cpp`.

---

## 5. Implicit Temporal Coupling in Renderer Initialization

`app.cpp` (line 46) calls `renderer.graphics_binding()` immediately after `renderer.init()` and passes it to `xr_session.create()`. Then line 47 calls `create_swapchains`, which is only valid after the session exists. The API offers no enforcement of this ordering — calling `graphics_binding()` before `init()` yields garbage with no diagnostic.

**Fix:** Make the ordering explicit, either via a builder pattern or by having `init()` return a capabilities struct (including the binding) that must be passed forward.

---

## 6. Scene Transform Ownership is Unclear

The fixed cube's transform is computed and stored in `scene.cpp` (lines 17-18). The hand cube transforms are computed in `app.cpp` (lines 71-82) and pushed into the scene via `set_controller_poses`, which takes raw `Eigen::Matrix4f*` pointers. If the caller frees those matrices before `cubes()` is called, it is a use-after-free. There is no clear rule for which layer owns transform computation.

**Fix:** Use value semantics for `set_controller_poses`. Move hand cube transform computation into scene or a dedicated child component so scene owns all transform logic.

---

## 7. Input Error Handling is Inconsistent

`input.cpp` has an `xr_ok` helper (lines 8-12) used inconsistently. Lines 20-35 log on failure; lines 37-55 return false silently. `sync()` (lines 78-102) silently skips updating poses on `xrLocateSpace` errors. The caller cannot distinguish stale pose from actively invalid pose.

**Fix:** Return a richer status from `sync()` (e.g., an enum distinguishing NO_TRACKING from SYSTEM_ERROR). Log all XR failures with component, function, and result code.

---

## 8. No Build-Time Enforcement of Architectural Boundaries

Design documents declare allowed dependencies, but CMake has no mechanism to enforce them. An accidental `target_link_libraries(renderer ... app)` compiles cleanly even though it violates the layering. The only guard is code review.

**Fix:** A script or custom CMake target that validates `target_link_libraries` calls against the allowed-dependency lists in each component's design doc.

---

## 9. Layer Submission Leaks into App Layer

`xr_session::render_frame` accepts a callback that returns `std::vector<const XrCompositionLayerBaseHeader*>` (xr_session.cpp line 115). `app.cpp` (line 91) assembles the projection layer directly. Adding a quad layer for UI requires changes at the app level even though layer submission is an XR concern.

**Fix:** Introduce a `LayerSubmitter` abstraction that `xr_session` queries, decoupling layer assembly from the app orchestration.

---

## 10. Inconsistent CMake Visibility

- `vr_math/CMakeLists.txt`: `openxr_loader` is PUBLIC but `vr_math.hpp` only uses OpenXR types — not exposed in the public API. Should be PRIVATE.
- `cube_mesh/CMakeLists.txt`: `GLESv3 log` is PUBLIC but these are implementation details in `.cpp`. Should be PRIVATE.
- `renderer/CMakeLists.txt`: `scene cube_mesh` PUBLIC (see finding #1).

Over-exposing dependencies inflates include chains, slows incremental builds, and obscures the real interface.

---

## 11. Scene Resize Pattern Creates Hidden Temporal Coupling

`scene.cpp` `update()` calls `cubes_.resize(1)`, then `set_controller_poses()` also calls `cubes_.resize(1)` before pushing hand cubes. `app.cpp` must call `update` before `set_controller_poses` or the hand cubes are silently erased. This ordering is not enforced or documented.

**Fix:** Separate the concerns explicitly — `update()` owns the world cube, `set_hand_cubes()` owns hand cubes, each managing an independent sub-range of the cube list (or separate data).

---

## 12. Hand Pose Validity Signaling is Ambiguous

`input.hpp` defines a `valid` bool on `HandPose`. `app.cpp` (lines 79-82) substitutes identity when `!lh.valid`. Nothing in the type system prevents forgetting the check, and there is no distinction between "not tracked this frame" and "tracking system error."

**Fix:** Use a `std::optional<HandPose>` or an enum status field. Make the absence of a pose explicit at the type level rather than a flag the caller must remember to inspect.

---

## 13. CubeMesh Header Carries GL Dependency

`gl_program.hpp` (included from `cube_mesh.hpp` line 3) includes `GLES3/gl3.h`, so any consumer of `cube_mesh.hpp` pulls in GL headers. If the renderer were ported to Vulkan, `cube_mesh.hpp` would need rewriting even though its interface is geometry-level.

**Fix:** Keep the `GlProgram` member in `cube_mesh.cpp` only; expose only `init()` and `draw(Eigen::Matrix4f)` in the header.

---

## Severity Summary

**Critical (breaks at scale):** #1 renderer coupling, #3 god object app  
**High (architectural violations):** #2 renderer owns content logic, #5 temporal coupling, #9 layer submission  
**Medium (maintainability):** #4 header pollution, #6 transform ownership, #7 input errors, #8 no boundary enforcement, #11 resize pattern, #12 validity signaling  
**Low (future-proofing):** #10 CMake visibility, #13 CubeMesh GL header
