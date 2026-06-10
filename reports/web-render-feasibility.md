# Feasibility Report: Web Browser Render Target with Remote/Local Pose Input

**Date:** 2026-06-06  
**Scope:** Extending the eyeballs node graph runtime to execute in a web browser, supporting two pose-input modes: (A) head pose streamed from a connected Quest 3, and (B) orientation from the phone IMU hosting the browser.

---

## Executive Summary

The node graph architecture is well-suited to this extension. The `signal_graph` runtime, node ABI, and port model are already platform-agnostic. The coupling to Android/OpenXR/EGL is localized to the platform layer (`FrameLoop`, `Renderer`, `XrSession`) and to source nodes that inject pose data. A web target is feasible but requires a parallel build toolchain (Emscripten), static node linking, a browser-native frame loop and renderer, and two new source nodes for pose input. No changes to the graph model or node ABI are required.

---

## Current Architecture: What Is Already Portable

| Component | Portable as-is | Notes |
|---|---|---|
| `signal_graph` (`Graph`, `tick_graph`, `parse_graph`) | Yes | Pure C++, no platform calls |
| `eyeballs_node_abi` / `make_descriptor<T>` | Yes | Eigen + boost::pfr compile under Emscripten |
| `component_registry` / `param_registry` | Yes | Pure C++ |
| `PortValue` variant (scalar, vec*, mat4, quat, audio) | Yes | |
| `GpuTexture` (carries `GLuint`) | Yes under Emscripten | Emscripten maps OpenGL ES → WebGL2; integer texture handles survive |
| `DrawFn` (`std::function<void(Matrix4f)>`) | Yes | Node draw calls become WebGL2 calls transparently |
| GLSL shaders in nodes | Minor tweaks | WebGL2 is GLSL ES 3.00; expect `precision` qualifier and extension adjustments per shader, not rewrites |

---

## Problems to Solve

### 1. Build Toolchain: Emscripten

The Android NDK CMake build must be paralleled by an Emscripten target. Emscripten provides:
- `emcc`/`em++` replacing the NDK clang
- OpenGL ES 2/3 → WebGL2 translation layer
- `libc++`, standard library, and most POSIX APIs

The Nix flake would gain an `emscripten` input and a new CMake toolchain file. Eigen and boost headers compile without modification. NDK-specific headers (`<android/log.h>`, `<android_native_app_glue.h>`, `<openxr/>`) must be excluded from the browser target via CMake conditions.

**Effort:** ~1–2 weeks. Mostly mechanical but requires iterating through per-node compile errors.

### 2. Static Node Linking

The `companion/` tooling (compile_node.py, freeze_graph.py) dynamically loads nodes via `dlopen`. WASM does not support arbitrary `dlopen` at runtime. All nodes must be statically linked into the WASM module at build time.

The node registry (`ComponentRegistry`) already supports static registration; the change is to the loading step, not the registry model. A build-time list of node types replaces the runtime `dlopen` calls. The graph JSON format and `parse_graph` are unchanged.

**Effort:** ~3–5 days.

### 3. Frame Loop Replacement

`FrameLoop` drives `xrWaitFrame`/`xrBeginFrame`/`xrEndFrame`. In the browser this becomes `requestAnimationFrame`. The replacement is a thin JS/Emscripten shim:

```
requestAnimationFrame(t) {
    tick_graph(graph, t / 1000.0);
    for each draw_call in graph.draw_calls: draw_call(viewProjection);
    graph.draw_calls.clear();
}
```

`FrameLoop` is already well-isolated. The browser shim can live in a new `platform/web/` directory alongside the existing `platform/android/`.

**Effort:** ~2–3 days.

### 4. Renderer Replacement

`Renderer` manages EGL context creation, XR swapchain allocation, and per-eye projection matrices. The browser equivalent:

- EGL → Emscripten canvas WebGL2 context (one call)
- XR swapchains → a single `<canvas>` element; two-viewport rendering for stereo, or single-viewport mono for phone use
- Per-eye projection matrices → standard `glm`/Eigen perspective matrix from a configurable FOV

The renderer interface (`on_draw(proj, view)` callback) already abstracts the projection and view matrices from the scene graph. The scene graph does not need to change.

**Effort:** ~1 week.

### 5. Pose Input: Mode A — Headset Streams to Browser

The Quest 3 runs the existing app. A lightweight WebSocket service (can extend the existing `companion.py`) publishes `XrPosef` (position + orientation) for the head each frame as JSON or binary. The browser graph contains a new `WebSocketPoseSource` node that:

- Connects to `ws://headset-ip:port`
- Parses incoming pose packets
- Emits `Quaternionf` (orientation) and `Vector3f` (position) on its output ports

The rest of the graph is unchanged; the view transform node reads from these ports exactly as it would read from the native XR source node.

Latency depends on network; over local WiFi expect 1–5 ms additional lag.

**Effort:** ~3–4 days (node + companion service endpoint).

### 6. Pose Input: Mode B — Phone IMU

Browsers expose orientation via the `DeviceOrientationEvent` API (iOS 13+/Android requires HTTPS + user permission gesture). It provides absolute orientation as Euler angles (alpha = azimuth, beta = elevation, gamma = roll) relative to a reference frame that varies by platform.

A `DeviceOrientationSource` node (implemented as a JS → WASM bridge):
- Registers a `deviceorientation` event listener in JS
- Converts alpha/beta/gamma to `Eigen::Quaternionf` (accounting for browser reference frame conventions)
- Emits `Quaternionf` on its output port

Platform reference frame differences between iOS and Android are a known complication; a calibration pass (tap to set forward) is advisable.

**Effort:** ~3–4 days (node + JS bridge + calibration).

---

## Risk Assessment

| Risk | Severity | Likelihood | Mitigation |
|---|---|---|---|
| Emscripten + Eigen/boost compile failures | Medium | Low | Both are well-tested under Emscripten |
| GLSL ES 3.00 shader incompatibilities | Low–Medium | Medium | Fix per-node; no architectural impact |
| `dlopen` replacement misses a node registration | Low | Low | Registry model already supports static init |
| `DeviceOrientationEvent` reference frame inconsistency (iOS vs Android) | Medium | High | Calibration step; document per-platform quirks |
| WebSocket latency makes Mode A feel laggy | Medium | Low–Medium | Local WiFi typically <5 ms; acceptable for casual use |
| WASM binary size (all nodes statically linked) | Low | Medium | Dead-code elimination (`-flto`, `--gc-sections`); nodes are small |

---

## What Does Not Need to Change

- `signal_graph.hpp` / `signal_graph.cpp`
- `eyeballs_node_abi.hpp` / `eyeballs_node_abi.h`
- `PortValue`, `Graph`, `Edge`, `NodeInstance` data model
- Graph JSON format
- Any existing node's logic, port names, or serialization
- `component_registry`, `param_registry`
- The companion graph editor (it already speaks JSON over HTTP/WebSocket)

---

## Recommended Sequencing

1. **Emscripten toolchain in Nix flake** — unblocks everything else; proves the core runtime compiles.
2. **Static node registry** — low risk, needed before integration testing.
3. **Browser frame loop + renderer** — minimal WebGL2 canvas, hardcoded camera, proves end-to-end render.
4. **Mode B (phone IMU)** — no external dependency; validates the input abstraction quickly.
5. **Mode A (WebSocket pose stream)** — builds on proven input abstraction; adds the companion service endpoint.

---

## Conclusion

The existing architecture requires no changes to its graph model or node ABI to support a web target. The work is additive: a new build toolchain, a browser platform layer (~2 weeks), and two new source nodes (~1 week). The largest unknown is per-node GLSL compatibility, which is incremental and low-risk. A working proof-of-concept with hardcoded camera and a single scene node is achievable in roughly two to three weeks of focused effort.
