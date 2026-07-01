# Performance Critique: VR Frame Loop and Renderer

## Critical Hot Path Issues

### 1. Projection matrix recomputed every frame per eye
**File:** `components/renderer/renderer.cpp` lines 207-210  
**Issue:** `projection()` calls are in the per-frame, per-eye rendering loop:
```cpp
for (int eye = 0; eye < 2; ++eye) {
    Eigen::Matrix4f proj = projection(views[eye].fov, 0.05f, 100.0f);
    Eigen::Matrix4f v    = view(views[eye].pose);
    for (const auto& cube : cubes) {
        Eigen::Matrix4f mvp = proj * v * cube.model;
```
The FOV is constant per eye per frame (it doesn't change between eyes). This recomputes the same 4x4 matrix twice per frame. Each `projection()` call does 12 transcendental operations (`tan()` calls on angles) and builds a 16-element matrix.

**Performance impact:** ~240 unnecessary FLOPs per frame at 90fps. Minor in isolation, but scales badly with eye count and is a pattern violation (cache the FOV per frame).

**Fix:** Cache projection matrix per eye before the cube loop. Store `projPerEye[2]` after locating views, reuse in the per-cube MVP computation.

---

### 2. Arbitrary matrix multiplications in the hot path without lazy evaluation
**File:** `components/renderer/renderer.cpp` lines 210, and `components/app/app.cpp` lines 81-82  
**Issue:** Dense matrix multiplications happen every frame for every cube:
```cpp
Eigen::Matrix4f mvp = proj * v * cube.model;  // 3 chained 4x4 multiplies
```
And in app.cpp:
```cpp
Eigen::Matrix4f lm = lh.valid ? (pose_to_world(lh.pose) * local_T * scale_m).eval() : ...
Eigen::Matrix4f rm = rh.valid ? (pose_to_world(rh.pose) * local_T * scale_m).eval() : ...
```
These trigger immediate `eval()` (explicit in hand case, implicit when stored). Each 4x4 multiply is 64 FLOPs. With 2 eyes, 1-3 cubes, and hand tracking, this is 64*3*cubes + 64*2*hands FLOPs per frame.

**Performance impact:** ~600+ FLOPs per frame at current scene complexity. Serious with hand poses computed every frame. At 90fps, this is 54k FLOPs/sec that could be deferred or computed on a secondary thread during raster.

**Fix:** 
- Cache projection*view product (`pv`) per eye; reuse as `pv * cube.model`.
- Defer hand pose model matrix computation to scene update (currently in the render callback). Store in `Scene` and reuse.
- Use Eigen's lazy evaluation; avoid `.eval()` on intermediate expressions.

---

### 3. XrLocateViews called per frame with no prediction or caching
**File:** `components/renderer/renderer.cpp` lines 166-182  
**Issue:** `xrLocateViews()` is an OpenXR synchronization point; it synchronizes the XR runtime's internal view cache. Called unconditionally every frame with `predictedDisplayTime`, but this is the *same* time for both eyes in the same frame. No caching across frames or early exit when validity unchanged.

**Performance impact:** OpenXR runtimes may hold locks or perform expensive tracking operations here. On Quest 3, this is typically <2ms in the critical path, but it's a serialization point that blocks all subsequent rendering until complete. If tracking is temporarily lost (validity flags = 0), the whole frame still does this work before detecting it.

**Fix:** 
- Early-exit check: if previous frame's view validity was 0 and `xrLocateViews` returns the same, skip rendering entirely.
- Profile the runtime's view latency; if >0.5ms, consider threading this call to background or interleaving with other work.

---

### 4. Scene::set_controller_poses() resizes vector every frame
**File:** `components/scene/scene.cpp` lines 27-29  
**Issue:**
```cpp
void Scene::set_controller_poses(...) {
    cubes_.resize(1);  // ALWAYS resizes to 1
    if (left_valid  && left_model)  cubes_.push_back(...);
    if (right_valid && right_model) cubes_.push_back(...);
}
```
This is called in the frame loop (app.cpp line 85). Every frame, it resizes the vector back to 1, then may push 0-2 more elements. This causes reallocation *every frame* if the vector capacity is exactly 1 (initial state). Even if capacity is 3, this is a redundant operation that invalidates the previous frame's span.

**Performance impact:** Vector::resize() can trigger allocation/deallocation on the hot frame path. At 90fps, this is a repeated allocation every single frame. Worst case: 90 allocations/sec on the audio thread context (Android main loop).

**Fix:** 
- Preallocate `cubes_` to capacity 3 during `Scene` construction. Change `set_controller_poses()` to directly assign pointers and set a count, not use resize/push_back.
- Or: separate the world cube from the hand cubes; maintain them in a fixed array or pre-allocated vector with an index marker.

---

### 5. Redundant EGL state management in cube_mesh::draw()
**File:** `components/cube_mesh/cube_mesh.cpp` lines 96-102  
**Issue:**
```cpp
void CubeMesh::draw(const Eigen::Matrix4f& mvp) {
    prog_.use();             // glUseProgram (GPU state change)
    prog_.uniform("uMVP", mvp);  // glGetUniformLocation + glUniformMatrix4fv
    glBindVertexArray(vao_);  // GPU state change
    glDrawElements(...);
    glBindVertexArray(0);     // GPU state change back to default
}
```
Called once per cube per eye (1-3 cubes * 2 eyes = 2-6 times/frame). Each call:
- Binds the same program twice (once via `use()`, implicitly again via the uniform call which requires the program to be active)
- Unbinds the VAO to 0 at the end (unnecessary state; next frame will rebind anyway)
- Calls `glGetUniformLocation` every draw (GPU driver cache hit, but still a function call and name lookup)

**Performance impact:** 10+ GPU state changes per frame that could be reduced to 2-3. Each state change costs cycles on the GPU command queue and CPU-GPU synchronization.

**Fix:**
- Move `prog_.use()` outside the cube loop in `Renderer::render_eyes()`. Call once per eye before the cube loop.
- Cache the uniform location or use a shader variant system if using dynamic uniforms.
- Remove the `glBindVertexArray(0)` unbind; it's defensive but unnecessary in VR where VAO state is managed tightly.
- Consider batching all cubes into a single instanced draw call with per-instance MVP matrices in a UBO.

---

### 6. Input::sync() called every frame without frame number deduplication
**File:** `components/input/input.cpp` lines 78-102  
**Issue:**
```cpp
void Input::sync(XrSession session, XrSpace worldSpace, XrTime time) {
    XrActiveActionSet active{actionSet_, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{...};
    syncInfo.activeActionSets = &active;
    syncInfo.countActiveActionSets = 1;
    xrSyncActions(session, &syncInfo);  // <-- OpenXR sync call

    for (int i = 0; i < 2; ++i) {
        xrLocateSpace(handSpaces_[i], worldSpace, time, &loc);
        ...
    }
}
```
`xrSyncActions()` is a heavy synchronization point in the OpenXR runtime. It processes all action state from the runtime and may block on controller input timing. Even if controllers are idle, this call is made unconditionally.

Called unconditionally every frame from `app.cpp` line 64, even during `APP_CMD_LOST_FOCUS` transitions.

**Performance impact:** 1-5ms per `xrSyncActions()` call depending on runtime and input hardware. On Quest, typically 0.5-2ms. This is a serialization point in the frame loop.

**Fix:**
- Gate `xrSyncActions()` behind a check: only call if `xrSession.should_render() == true`.
- Cache hand poses and only re-sync on state changes or every Nth frame if input is not needed every frame.
- Profile: measure `xrSyncActions()` latency on target hardware; if >1ms, consider moving to a background thread with thread-safe pose exchange.

---

### 7. GL uniform lookup by name string every draw call
**File:** `components/gl_program/gl_program.cpp` lines 50-56  
**Issue:**
```cpp
void GlProgram::uniform(const char* name, const Eigen::Matrix4f& mat) const {
    GLint loc = glGetUniformLocation(id, name);  // Name-based lookup EVERY TIME
    if (loc < 0) {
        __android_log_print(..., "Uniform not found: %s", name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
}
```
`glGetUniformLocation()` is a string lookup in the program's uniform symbol table. It's cached by the driver, but it's still a function call and name comparison every draw. Called once per cube per eye with the string "uMVP".

**Performance impact:** 60+ function calls per frame (2-6 draws * 90fps). Minor in absolute terms (~100µs), but avoidable.

**Fix:** Cache uniform location at shader build time. Store `GLint mvp_loc` in `GlProgram` after link; expose a `uniform_mat4(GLint loc, const Eigen::Matrix4f&)` variant that takes the cached location.

---

### 8. Scene::update() performs three separate sinf/cosf operations per frame
**File:** `components/scene/scene.cpp` lines 4-18  
**Issue:**
```cpp
void Scene::update(double time) {
    float t = static_cast<float>(time);
    float ax = 0.4f * sinf(t * 0.19f + 0.0f);  // sinf #1
    float ay = 0.4f * sinf(t * 0.27f + 1.1f);  // sinf #2
    float az = 0.4f * sinf(t * 0.13f + 2.3f);  // sinf #3
    Eigen::Quaternionf q =
        Eigen::AngleAxisf(ax, Eigen::Vector3f::UnitX()) *  // Creates 3 rotations
        Eigen::AngleAxisf(ay, Eigen::Vector3f::UnitY()) *  // Each builds a 3x3 matrix
        Eigen::AngleAxisf(az, Eigen::Vector3f::UnitZ());   // Then multiplies them: 27 FLOPs
    Eigen::Affine3f xf = 
        Eigen::Translation3f(...) * q * Eigen::Scaling(...);  // More multiplies
    cubes_.resize(1);
    cubes_[0].model = xf.matrix();  // Convert Affine to Matrix4f
}
```
Three transcendental function calls. Three `AngleAxisf` constructions (each ~10 FLOPs). Two matrix multiplications for the quaternion composition (27 FLOPs each). One Affine-to-Matrix conversion. Total: ~100+ FLOPs.

**Performance impact:** Minor per-frame (negligible), but called unconditionally 90 times/sec. Not a hot path bottleneck yet, but the pattern (expensive animation) will scale badly if more objects animate.

**Fix:** 
- Precompute keyframe animations or use lookup tables for the sine waves (a ~360-element LUT would replace 3 sinf calls with 3 array lookups).
- Separate rotation from translation; apply rotation separately to avoid the final `xf.matrix()` conversion if only using the rotation part.
- For future: consider a dedicated animation component that batches all updates and uses SIMD.

---

### 9. No frame pacing or frame drop detection
**File:** `components/xr_session/xr_session.cpp` lines 114-151  
**Issue:** The frame loop has no explicit frame budget accounting:
```cpp
void XrSessionObj::render_frame(...) {
    XrFrameWaitInfo waitInfo{};
    XrFrameState frameState{};
    XrResult r = xrWaitFrame(handle, &waitInfo, &frameState);
    // ...
    if (frameState.shouldRender && on_render)
        layers = on_render(frameState.predictedDisplayTime);
    // ...
    r = xrEndFrame(...);
}
```
`xrWaitFrame()` blocks until the next frame slot is ready (at 90fps, every ~11ms). But there's no measurement of how long `on_render` takes. If it exceeds the frame budget (11ms), the next `xrWaitFrame` will already be overdue, causing frame drops with no visibility.

The `xrEndFrame()` failure is logged at most once every 2 seconds (throttled, line 143-145), so repeated frame drops go silent.

**Performance impact:** Frames are dropped silently. Users see jitter/judder. At 90fps, 11.1ms budget must include: xrLocateViews (0.5-2ms), view acquire (0.1ms), render (1-3ms), view release (0.1ms), xrSyncActions/Input (0.5-2ms), scene update (0.01ms), matrix math (0.1ms), xrEndFrame (0.5-1ms). Total: ~4-12ms currently feasible; any scene complexity increase will exceed budget silently.

**Fix:**
- Add frame budget profiling: measure and log frame time. If `on_render` duration + XR overhead > 9ms (80% of budget), log a warning with a frame drop counter (once per 100 frames).
- Implement frame drop recovery: if `xrEndFrame` fails, immediately start the next frame without trying to render.
- Expose a frame timing API to allow scene/renderer to make adaptive choices (LOD, draw call reduction, etc.).

---

### 10. Vector of VR poses stored in scene, modified every frame
**File:** `components/renderer/renderer.hpp` line 12 and `components/scene/scene.hpp` line 17  
**Issue:** The `cubes_` vector in `Scene` is a `std::vector<CubeInstance>` that's resized every frame in `set_controller_poses()` (item 4 above). But it's also accessed as a span in `render_eyes()`, and the span is passed by const reference. This creates a lifetime and invalidation hazard if the vector is resized between reads.

Currently safe because read happens immediately, but the pattern is fragile. If future code caches the span or accesses it after a frame update, this will crash.

**Performance impact:** Not immediate, but a UAF hazard. Scales badly if hand poses are added to the queue asynchronously.

**Fix:** Use a fixed-size array or pre-allocated vector with explicit element counts. Change `set_controller_poses()` signature to take counts, not bools. Example:
```cpp
void Scene::set_controller_poses(const Eigen::Matrix4f* left, const Eigen::Matrix4f* right);
void Scene::set_cube_count(int count) { cubes_.resize(count); }  // Done once on init
```

---

## Design Issues That Will Become Critical

### 11. No texture support or batching infrastructure
The current renderer has a single `CubeMesh` that draws all cubes in a loop with per-draw calls. Adding textures or more complex geometry will require:
- Per-mesh FBO bind (currently implicit: one FBO per eye per swapchain image)
- Per-mesh texture bind(s)
- Per-draw uniform updates

At >10 objects, this becomes 20-40 GL state changes per eye per frame, exceeding modern mobile GPU budgets.

**Fix:** Build a renderer abstraction now: a "DrawCall" struct (mesh, material, MVP, bounds) and a batch renderer that sorts by material and mesh, minimizing state changes.

---

### 12. No level-of-detail (LOD) or frustum culling
`render_eyes()` loops through all cubes unconditionally (line 209):
```cpp
for (const auto& cube : cubes) {
    Eigen::Matrix4f mvp = proj * v * cube.model;
    cube_mesh_.draw(mvp);
}
```
With 100 cubes, this becomes 200 draw calls per frame. No LOD, no view frustum test to skip distant cubes.

**Fix:** Add a simple frustum cull before the loop. Store cube bounds with the model matrix; cull against the near/far planes and FOV bounds.

---

### 13. Swapchain image acquire/release has no error recovery
**File:** `components/renderer/renderer.cpp` lines 192-198  
**Issue:**
```cpp
XrSwapchainImageAcquireInfo ai{};
uint32_t index = 0;
XR_LOG_ERR(xrAcquireSwapchainImage(e.handle, &ai, &index));

XrSwapchainImageWaitInfo wi{};
wi.timeout = XR_INFINITE_DURATION;  // <-- INFINITE WAIT
XR_LOG_ERR(xrWaitSwapchainImage(e.handle, &wi));
```
`xrWaitSwapchainImage()` with `XR_INFINITE_DURATION` will block indefinitely if the swapchain is starved (e.g., XR runtime is slow). This can hang the entire app. If acquire fails, `index` is uninitialized; the subsequent `e.fbo(index)` dereferences undefined memory.

**Performance impact:** Potential hard hang. Worst-case: app becomes unresponsive.

**Fix:**
- Use a finite timeout (e.g., 5ms, 50% of frame budget): if timeout, skip rendering this eye and log a warning.
- Check `xrAcquireSwapchainImage()` result before using `index`; skip the eye if it fails.

---

### 14. No async readback or deferred GPU tasks
All GPU work is synchronous in the frame loop. If the GPU is slower than the CPU, the frame loop will stall waiting for GPU results. This is unavoidable in general, but:
- No double/triple buffering of GPU command buffers
- No command list recording to defer GPU submission
- No async texture uploads or resource streaming

This limits future scalability.

**Fix:** Not needed now, but plan for it: consider OpenGL KHR_debug to detect GPU stalls; profile GPU vs CPU time independently.

---

## Summary Table

| # | Issue | Impact | Effort | Priority |
|---|-------|--------|--------|----------|
| 1 | Projection matrix recomputed per eye | 240 FLOPs/frame | Low | Medium |
| 2 | Arbitrary matrix multiplies in hot path | 600+ FLOPs/frame | Medium | High |
| 3 | XrLocateViews no caching | 0.5-2ms serialization | Medium | Medium |
| 4 | Scene::set_controller_poses() resize | Per-frame allocations | Low | High |
| 5 | Redundant EGL state in cube draw | 10+ state changes/frame | Low | Medium |
| 6 | Input::sync() unconditional | 0.5-2ms latency | Low | Medium |
| 7 | GL uniform lookup by name | 60+ calls/frame | Low | Low |
| 8 | Scene::update() transcendentals | 100+ FLOPs/frame | Low | Low |
| 9 | No frame pacing/drop detection | Silent frame drops | Medium | High |
| 10 | Vector invalidation hazard | UAF risk | Low | Medium |
| 11 | No texture/batch infrastructure | Will fail >10 objects | High | High |
| 12 | No LOD/culling | N^2 complexity | High | Medium |
| 13 | Swapchain acquire infinite wait | Potential hang | Medium | High |
| 14 | No async GPU work | Low scalability | High | Low |

## Immediate Actions (Before 100 Objects)

1. **Fix #4:** Change `Scene::set_controller_poses()` to avoid resize every frame.
2. **Fix #9:** Add frame timing instrumentation and frame drop detection.
3. **Fix #2:** Cache `proj*v` and defer hand pose computation.
4. **Fix #13:** Replace infinite timeout with finite timeout + error handling.
5. **Fix #5:** Move `prog_.use()` outside cube loop; batch draws.

These address the most likely frame drops and latency spikes under load.
