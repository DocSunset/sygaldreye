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

Slice 1 COMPLETE (2026-06-10, later):
- `sh/agent/`: launch/stop/screenshot/camera/graph/values/controller.sh
- GET /values port-value probe; POST /controller virtual hands
- Lazy GL init across all visual nodes (graphs parsed on HTTP thread got
  GL-less nodes — every live-added node was invisible, Quest included)
- vr_editor runs ON HOST (it was never XR-bound — only XrPosef types);
  text_mesh ported (android log → log_component)
- Agent drove the editor end to end: virtual trigger over palette spawned a
  live lissajous node. Proven by screenshot.

More bugs found & fixed:
- palette spawn inserted nodes into the EDGES array (json.rfind) — palette
  spawning never worked on any platform
- to_json emitted invalid JSON for vector ports ("light_dir":,) — every
  /graph response was unparseable by strict parsers
Logged to kanban/backlog: text_mesh glyph corruption, editor card layout.

## 2026-06-10 (cont.) — Decomposition slice: spine graphified

Architecture now (host):
- migrate_graph: live graph swaps adopt node state by (id, descriptor);
  fresh params re-applied. Editor lives in the graph it edits and survives
  its own edits. THE enabling primitive for live patching.
- Nodes: fly_camera (app renders with values["camera.pv"]), hand (HTTP-fed
  on host / XR-fed on device), editor (hand poses via ordinary edges),
  lfo/scale/add/mul, udp_send/udp_recv (loopback bridge; string params
  needed for cross-device addressing).
- host_app seams that remain C++: param queue (POST /param → deserialize on
  render thread), editor context injection (set_context/take_edit), camera
  aspect pump, final draw using camera.pv, screenshot.
- assets/graphs/*.json auto-register as subgraph node types ("plugins in
  JSON"). orbit_cam.json = adds + lfos; proven driving a cube live.
- Two-instance UDP bridge proven: B's cube bobs on A's lfo (channel 0,
  port 9100, ~1 frame lag).
- parse_graph accepts standard (whitespace) JSON now.
- "add" node doubles as a constant source (a unwired, b=value) — inlet
  fan-out inside subgraphs uses this trick; a dedicated const node would
  read better.

## 2026-06-10 (cont. 2) — first node decomposition shipped

- sky_dome → sky_dome (gradient+sun) + star_field node; assets/graphs/sky.json
  recomposes them. Duplicate-name inlets fan out one external port to several
  inner ports (no passthrough node needed).
- Bugs: ABI setters matched only display names (spaces) while params match
  field names — edges using field names silently no-oped (now both accepted;
  kanban: canonicalize). star_field xyww depth==1.0 failed GL_LESS — stars
  were likely never visible on Quest either; drawn depth-off now.
- Night-sky screenshot proves the whole chain: subgraph type from disk,
  edge-driven inlet fan-out, stars from the split-out node.

## 2026-06-10 (cont. 3) — UI as graph nodes

- ui_slider/ui_button/ui_pane (components/ui_nodes): widgets are nodes;
  hand ray + trigger arrive via edges; values + draw calls flow out.
  Slider drag state is node-internal → survives migration. VrPanel does
  the ray→uv hit math.
- assets/graphs/control_panel.json: pane + 2 sliders as a subgraph plugin
  (ray/trigger inlets fan out; value outlets). Agent dragged sliders via
  controller.sh; values verified through smooth → cube.scale.
- math primitives: const, sub, div, phasor, smooth join lfo/scale/add/mul.
- Path to editor-as-subgraph: palette = ui_pane + per-row ui_button grid;
  node cards = panes + port ui_buttons + ui_sliders; wires need a
  wire-render node + drag state machine node. Text labels need text_label
  on host (text_mesh already ported — small lift). The C++ vr_editor
  remains as scaffolding until those exist.

## 2026-06-10 (cont. 4) — RD split: textures flow over edges

- rd_gpu (sim) ported to host + registered both platforms; rd_renderer
  lazy-creates. sim.concentration → view.texture proven (Gray-Scott
  patterns on host). Any texture-input node can now consume the sim.
- GL state discipline lesson for ALL FBO nodes: own VAO, save/restore
  framebuffer + viewport, never bind FBO 0 (headless renders into an FBO).
  rd_gpu violated all three; fixed. Float32 textures need NEAREST in core
  GLES3 (Adreno extension hid this).

Next:
- water decomposition (mesh/shader split)
- text_label node on host (labels for graph-built UIs)
- editor-as-subgraph increments: palette from ui_buttons first
- GL-state audit of remaining FBO users when they become nodes
- string PortValue/params (UDP host addressing, labels, JSON events)
- editor deep bug hunt: wire-drag (grip), sliders, dwell-delete, undo
- Android app.cpp: adopt migrate_graph + hand/editor nodes (currently still
  the old C++ editor member; works but diverges from host architecture)
- Quest on-device validation pass
- update planning/vision.md slice plan as slices merge
