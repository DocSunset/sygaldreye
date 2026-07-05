# Linux Host Version of Sygaldreye (Spectator App)

The repository contains a **"Spectator" application** — a desktop/Linux host build of the VR dataflow application that runs on standard x86_64 Linux instead of on Meta Quest 3 hardware. Here's a comprehensive breakdown:

## What It Is

The **Spectator app** (`/home/tw/tw/repos/sygaldreye/app/spectator/spectator_main.cpp`) is a portable desktop shell around `PeerCore`, the platform-agnostic heart of Sygaldreye. It's the non-XR companion to the Quest app, enabling:
- Live graph editing and node patching on desktop
- Agent-operable camera control and interaction (FPS-style with mouse/keyboard)
- HTTP-driven control surface for LLM agents
- Headless operation for CI/testing via `--headless` flag
- Screenshot capture and graph inspection

As stated in `/home/tw/tw/repos/sygaldreye/planning/vision.md` (lines 70–74):
> "Agent-operable host app — spectator app on Linux; mouse/keyboard + agent-drivable camera/interaction source nodes; generic probe / texture-tap / audio-tap nodes over HTTP; `sh/agent/` script kit (look/move/click/screenshot/probe)."

## Architecture & Components

**Three host-specific components:**

1. **`host_gl_window`** (`/home/tw/tw/repos/sygaldreye/components/host_gl_window/`)
   - GLFW-based windowing for desktop
   - Creates OpenGL ES 3.2 context via EGL (matching Android/Quest's GLES 3.2)
   - Renders to on-screen framebuffer with vsync
   - Entry point: `HostGlWindow::create()` → `run(RenderCallback)`

2. **`host_gl_context`** (`/home/tw/tw/repos/sygaldreye/components/host_gl_context/`)
   - Headless EGL context setup (no window)
   - Creates an offscreen PBUFFER surface
   - Used by `--headless` mode and for unit tests
   - Also powers `scene_snapshot` tests via `host_gl_context`

3. **`host_app`** (`/home/tw/tw/repos/sygaldreye/components/host_app/`)
   - Desktop shell: registers the full node vocabulary (renderer, editor UI, audio, math, etc.)
   - `void init(int http_port)` — registers ~100+ node types at startup
   - `void frame(int width, int height, double time_s)` — pump graph, render via camera.view/proj
   - Reads camera transform from the graph, renders via `RenderRegion`
   - **`host_input`** — FPS-style camera control: WASD+QE movement, right-mouse look

**Shared core (`PeerCore`):**
- Location: `/home/tw/tw/repos/sygaldreye/components/peer_core/`
- Portable across all platforms (host, Android, planned WASM)
- Handles: component registry, live/pending graph swap, HTTP server, parameter queues, screenshot fulfillment, remote node proxies

## How XR Is Stubbed / Differences from Quest

**Quest (Android) build** (`CMakeLists.txt` lines 131–158):
- Conditional: `if(CMAKE_CROSSCOMPILING)` — Android NDK toolchain
- Includes: `xr_sources`, `xr_session`, `frame_loop`, `input`, `egl_context`, `eye_swapchain`, `renderer`, `app`
- Full OpenXR + eye tracking + hand controllers

**Host (Linux) build** (`CMakeLists.txt` lines 1–130 + lines 122–124):
- Conditional: `if(NOT CMAKE_CROSSCOMPILING)` — native host compile
- **No XR:** skips `xr_sources`, `xr_session`, `renderer_node` entirely
- **Instead:** uses desktop GL rendering (`host_gl_window`, `host_gl_context`)
- **Input:** GLFW keyboard/mouse instead of OpenXR action sets
- **Rendering:** Single on-screen viewport, driven by graph's camera node (view/proj matrices read from `"camera"` node outputs)
- **Audio:** Full audio graph (DAC, synth, spatial nodes) — desktop audio playback via OS
- **Graphs:** Loads the same default graph JSON as Quest, but can skip XR-specific nodes

## Build & Run

**Build for host:**
```bash
nix develop --command bash -c \
  'cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Debug && \
   cmake --build build/host'
```

**Run windowed:**
```bash
./build/host/app/spectator/spectator
```
- Opens 1280×720 window
- WASD+QE to move camera, right-mouse to look
- HTTP server on port 8080 (default)

**Run headless (agent/CI):**
```bash
./build/host/app/spectator/spectator --headless --port 8080
```
- Renders to 1280×720 offscreen FBO via EGL (no window)
- Same HTTP surface for agent control
- ~60 fps (16ms per frame)

**Run tests:**
```bash
sh/test_host.sh
```
- Builds all host unit tests (gtest binaries in `build/host/components/*/component_*_test`)
- Requires `host_gl_context` (each test runs in own EGL context)

## Files Involved

**Entry point & main window loop:**
- `/home/tw/tw/repos/sygaldreye/app/spectator/spectator_main.cpp` — `int main(int argc, char** argv)`

**CMake targets:**
- `/home/tw/tw/repos/sygaldreye/CMakeLists.txt` — conditional host/Android splits
- `/home/tw/tw/repos/sygaldreye/app/spectator/CMakeLists.txt` — `add_executable(spectator ...)`
- `/home/tw/tw/repos/sygaldreye/components/host_app/CMakeLists.txt` — host_app lib (conditional `if(NOT CMAKE_CROSSCOMPILING)`)
- `/home/tw/tw/repos/sygaldreye/components/host_gl_window/CMakeLists.txt` — GLFW linkage
- `/home/tw/tw/repos/sygaldreye/components/host_gl_context/CMakeLists.txt` — EGL + headless

**Core abstractions:**
- `/home/tw/tw/repos/sygaldreye/components/peer_core/peer_core.hpp` — portable app core
- `/home/tw/tw/repos/sygaldreye/components/host_app/host_app.hpp` — desktop shell API

**Windowing & context:**
- `/home/tw/tw/repos/sygaldreye/components/host_gl_window/host_gl_window.hpp/.cpp` — GLFW window + ES 3.2 context
- `/home/tw/tw/repos/sygaldreye/components/host_gl_context/host_gl_context.hpp/.cpp` — headless EGL setup

**Input handling:**
- `/home/tw/tw/repos/sygaldreye/components/host_app/host_input.hpp/.cpp` — WASD/mouse FPS controls

**Build scripts:**
- `/home/tw/tw/repos/sygaldreye/sh/build.sh` — Android build (separate `build/android` dir)
- `/home/tw/tw/repos/sygaldreye/sh/test_host.sh` — run all host gtest binaries

**Documentation:**
- `/home/tw/tw/repos/sygaldreye/planning/vision.md` — vision & slice plan (lines 70–74 explain host app)
- `/home/tw/tw/repos/sygaldreye/planning/sprint-3-integration-tests.md` — host build + gtest setup
- `/home/tw/tw/repos/sygaldreye/README.md` — "Linux and Quest 3 native"

## Key Insight

The host and Android apps are **the same application**, differing only in:
1. **Registered nodes** — host registers editor UI nodes, Android registers VR/input nodes
2. **Platform seams** — window (GLFW vs Android NativeActivity), GL context (EGL host vs EGL Android), input (GLFW vs OpenXR), audio (OS vs Android AudioTrack)
3. **Conditional builds** — host skips XR SDK entirely via `CMAKE_CROSSCOMPILING` check

The architecture is explicitly designed for future **WASM/browser peer** (stated in vision line 66: "Browser peer constraints apply now"). The host demonstrates how to cleanly swap rendering, input, and audio without touching the core graph engine.
