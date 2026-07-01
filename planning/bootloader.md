# Graph executor bootstrapping — assessment

Status: assessment, 2026-07-01. Precedes implementation and the later node-factoring pass.

## Target

The entry point of every non-frozen sygaldreye program is a portable graph-executor
bootloader. Stage 0 is the only native C++ outside the graph execution system, and it is
identical on every platform. Everything platform-specific is expressed as:

- platform-specific **nodes** that simply aren't linked/registered elsewhere, and
- platform-specific **graphs** selected by observing the executor's environment.

## Where we are

Closer than it looks. `PeerCore` already is "the portable peer minus the platform"
(peer_core.hpp:21-26); `signal_graph`, `component_registry`, and `parse_graph` have no
platform dependencies; platform audio/net differences already live in per-target sources
selected by CMake (5 `#ifdef` blocks total in the whole tree); the block region already
demonstrates the key pattern — the `dac` **node** owns the audio device and paces its
region.

What violates the target today:

| Shell | Native outside the graph | LOC (approx) |
|---|---|---|
| Quest (`components/app`) | `android_main`, XR instance/system polling (xr.cpp), `XrSessionObj`, `FrameLoop`, `Renderer`+`EglContext`+`EyeSwapchain`, `Input` + `pump_xr_sources()`, `Scene`, mdns start, JNI multicast lock, hand-listed registry (92 `register_builtin`) | ~990 |
| Host (`app/spectator`, `host_app`) | GLFW/EGL window or headless FBO, fps input polling, main loop, hand-listed registry (106) | ~265 |
| Web (`app/web`) | WebGL context, DOM input, rAF loop + 250ms net pump, its own default graph, its own inline registry (41) | ~291 |

Cross-cutting violations:

1. **Three divergent hand-maintained node registration lists.** Capability should be
   "what is linked into this target", not what a shell remembered to register.
2. **The shell drives the executor**, calling `begin_frame → pump_contexts → tick →
   collect_edits` from a platform-owned loop (XR frame loop / GLFW / rAF). The frame
   region has no equivalent of `dac`.
3. **Graph selection is hardcoded** (`kEditorGraph` compiled into peer_core; web has a
   different `kDefaultGraph`). No environment observation, no staged loading.
4. **Host services wired outside the graph**: HTTP server and edit/param queues owned by
   PeerCore, mdns started in `android_main`, `graphs_dir` plugin scan in `PeerCore::init`,
   XR poses pushed into nodes from outside by `pump_xr_sources`.

## Architectural decisions required

**D1 — Pacer inversion (the hard one).** Someone must call `tick`. Today xrWaitFrame /
GLFW / rAF block-and-callback from the shell. Proposal: stage 0 owns a trivial loop;
a *pacer seam* lets one node per peer claim frame pacing, exactly as `dac` claims block
pacing. Platform pacer nodes: `xr_frame_pump` (Quest — owns session, wait/begin/end,
layer submission), `gl_window` (host — GLFW pump), `raf_pump` (web — emscripten main
loop). Until a pacer claims the seam, stage 0 free-runs offline ticks — which is exactly
what app.cpp's pre-headset loop and doffed-headset path already do by hand.

**D2 — Entry symbol honesty.** `android_main(android_app*)`, `int main()`, and
emscripten's main-loop registration cannot literally be one function. The invariant worth
enforcing: one shared stage-0 TU (`bootloader.cpp`), and per-platform entry *trampolines
of <10 lines* whose only job is to call it. The trampolines are part of the bootloader
component, not of any app.

**D3 — Registration = linkage.** Replace the three lists with self-registering
descriptors (static registrar per node TU) or a CMake-generated registration TU per
target. Watch linker dead-stripping (Android `.so`, wasm): needs whole-archive or the
generated-TU approach. Then the bootloader never names a node.

**D4 — Environment observation = registry probing.** Platform-graph selection should ask
"is node type `xr_frame_pump` registered? is `gl_window`?" rather than sniff OS strings.
Capability-based selection falls out of D3 for free and works for degraded environments
(Quest build with no headset attached behaves like its capabilities, not its brand).

**D5 — Resource ordering across stages.** GL nodes assume a current context at
create/first-tick. Staged loading must guarantee the stage-2 context/pacer node is live
before any GL node initializes. Existing lazy-GL patterns (render_region caches,
"call init after a context exists" in peer_core.hpp:37-38) point the way: GL resource
acquisition on first tick, never in `create()`.

## Bootstrap stages (proposed)

- **Stage 0 (native, identical everywhere)**: registry from link set (D3) → parse
  embedded stage-1 boot graph → loop `begin_frame/tick/collect_edits` until quit,
  yielding to a pacer when one claims the seam (D1).
- **Stage 1 (embedded boot graph, portable JSON)**: capability probe nodes → select
  platform bootstrap graph → `graph_loader` node swaps it in. `graph_loader` is
  PeerCore's existing `/graph` swap machinery exposed as a node (resource-holder context
  seam, same as `graph_source`).
- **Stage 2 (platform graph, chosen not compiled)**: instantiates the pacer/context/device
  nodes for this environment (`xr_frame_pump`+`egl_context` / `gl_window` / `raf_pump`+
  `webgl_context`), plus `http_server`, `mdns_advertise`, `plugin_scan` nodes; then loads
  stage 3.
- **Stage 3**: the application graph (today's editor).

## Work slices, in order

1. **Self-registration** (D3). Mechanical; deletes ~240 registration calls and the
   web/host/Quest drift. Prerequisite for everything.
2. **Extract stage-0 bootloader from PeerCore + pacer seam; host first.** `gl_window`
   pacer node; `spectator_main.cpp` becomes a trampoline. PeerCore sheds `Config`
   (default_graph_json/graphs_dir/http_port all move graph-ward in later slices).
3. **Boot graphs** (D4). `capability_probe` + `graph_loader` nodes, embedded stage-1
   graph, platform graphs as data. Kill `kEditorGraph`-as-the-entry and web's separate
   default.
4. **Quest nodification** (the bulk): `xr_frame_pump` node absorbing xr.cpp +
   XrSessionObj + FrameLoop + Renderer/EglContext/EyeSwapchain; input polling moves
   inside `head_pose`/`controller` nodes (delete `pump_xr_sources`); retire `Scene`;
   `mdns_advertise` node (owns the JNI multicast lock on Android).
5. **Web unification** onto the same bootloader (`raf_pump`, fetch-based `plugin_scan`
   variant — no dlopen/fs on web, fine under "platform-specific nodes").
6. **Host services as nodes**: `http_server`, `plugin_scan`; PeerCore dissolves into
   bootloader + nodes.

Then the second step — factoring the nodes — starts from a position where *everything*
is a node, which is the forcing function working as intended.

## Risks

- **D1 timing fidelity**: xrWaitFrame prediction must still gate rendering; the pacer
  node owns wait/begin/end internally, so the graph ticks inside the XR frame envelope as
  today — but getting the seam wrong shows up as judder. Prove on host (GLFW) first.
- **Static-init/linker traps** in D3 on `.so` and wasm targets.
- **Stage-2 ordering** (D5): a stage graph that references GL before its context node is
  live must fail loud at parse/plan time, not crash in a driver.
- **HTTP-as-node** moves the control surface into graph content: an edit that removes the
  http node saws off the branch it sits on. Boot graphs should be re-loadable from stage 1
  on such failure (watchdog in stage 0).
- Frozen programs are unaffected: freezing (kanban/backlog/freezer.md) bypasses the
  bootloader by design — "non-frozen" is exactly the right scope qualifier.
