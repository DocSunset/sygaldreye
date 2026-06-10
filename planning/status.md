# Status

_Keep this current. Vision and slice plan: `planning/vision.md`._

## 2026-06-10 — Slice 1 in progress

Done:
- Host build green (was broken: signal_graph/component_registry/subgraph_node
  include+link cycle; fixed with `subgraph_descriptor_fwd.hpp` out-of-line
  deleter + honest CMake static-lib cycle). All 21 graph tests pass on host.
- Host app spine: `app/spectator` rewritten around new `host_app` component.
  Live graph + HTTP on configurable `--port`: GET/POST `/graph`, `/palette`,
  GET/POST `/camera`, POST `/screenshot` (frame-synchronized PNG), `/quit`.
  `--headless` runs via EGL surfaceless + FBO; windowed mode is GLFW with
  GLES 3.2 EGL context (matches Quest) + WASD/right-drag fly camera
  (`fly_camera`, `host_input`).
- First light verified headless: sky + Gerstner water + lit cube, 0 errors.
  Telegrammed to Travis.

Bugs found & fixed (all latent on Quest as well):
1. GLSL helper prepended before `#version` (lit_shader, water_surface) —
   Adreno tolerated, Mesa rejects. Insert before `void main` instead.
2. `sky_dome` + `water_surface` never init GL when instantiated via graph
   (static `create()` factory never called by `desc->create()`); added lazy
   `init_gl()` on first tick. Suspect other visual nodes share this — audit.

Gotchas:
- Run host binaries with `LD_LIBRARY_PATH=/usr/lib` (nix loader doesn't search
  system GL); launch outside `nix develop`.
- Port 8080 occupied on Travis's box (node process) — use e.g. 8930.
- `pkill -x spectator` (-f matches your own shell).
- Some nodes compile shaders in constructors → GL context must exist before
  `HostApp::init` (`parse_graph`, `/palette` create temp instances too).

Next:
- `sh/agent/` script kit (launch/screenshot/look/move/graph)
- value-probe endpoint (read graph `values` map over HTTP)
- audit remaining visual nodes for the init_gl bug (aurora, chladni, etc.)
- editor on host + bug hunt (task list in session; kanban for found bugs)
