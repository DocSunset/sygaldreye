# Chapter 7 — Capability packages (PKG)

*A builder implementing this chapter delivers: the package shape (vocabulary +
passes + machinery), the audio package as the reference retrofit, and the XR,
render, worker, store, and net packages on the same shape.*

## Design

An executor flavor is **a world apart** — its own app/engine/execution triple —
spliced into the main engine at runtime when its capability is observed. Three
parts, always:

1. **Vocabulary** — its node types (dac/adc, xr_session, draw, store_*,
   ws/mdns...), present only where linked (sz.generated_registration's manifest is the capability
   claim).
2. **Passes** — wired into the engine's published fan-ins: recognize its
   region in an app graph (dac-closure inference, generalized), construct its
   execution subgraph and context, choose boundary adapters. The dac/adc
   "special contract" is just this package's rewrite rule binding those node
   types to its context.
3. **Machinery** — C++ context internals (thread, device callback, frame
   envelope, event loop) inside its executor natives. The executor owns pacing
   and the platform resource; inner node families bind through the package's
   contract — no singleton reach.

Splicing a package is an ordinary engine-graph edit driven by environment
observation. **Placement**: when a package is absent locally, compilation may
place that region on a peer advertising the capability (net adapters +
remote-peer machinery) — Quest compiles its worker region onto the Linux peer;
a browser peer's XR region compiles onto the Quest and the browser becomes a
spectator. Cross-network placement is a fallthrough of compilation, not a
feature.

Package inventory and their existing-code seeds:

| Package | Vocabulary | Machinery seed (exists) |
|---|---|---|
| audio | dac, adc, ugens | AudioEngine/AudioRegion/audio_output |
| render | draw, render_head, shader nodes | render_region, GlProgram/GlGeometry |
| xr | xr_session, head_pose, controllers | xr.cpp, XrSessionObj, FrameLoop, Renderer/Egl/EyeSwapchain (to be absorbed) |
| worker | worker jobs, derivation runner | components/worker |
| store | store graph face, refs, query | data_dir machinery (ch. 3) |
| net | ws/mdns nodes, net mappings, proxies | ws_link, mdns_advertiser, remote_node |

## Requirements

**pkg.package_shape** Every package ships exactly
vocabulary + passes + machinery; passes integrate via fan-ins only
(cmp.extension_ports); machinery is reachable only through the package's executor and
contract.
- pkg.package_shape.omittable: a package can be omitted from a target at build time and the
  system boots with that vocabulary simply absent from the palette.

**pkg.audio_retrofit** AudioRegion becomes the audio
package with no behavior change.
- pkg.audio_retrofit.audio_suite_green: full existing audio test suite green after the retrofit; region
  membership (dac closure), latch/snapshot/ring semantics, and pump_offline
  identical (exe.region_inference, exe.canonical_mappings, exe.executor_contract.headless_computes re-run unchanged).

**pkg.xr_package** Absorbs xr.cpp + XrSessionObj + FrameLoop + Renderer/
EglContext/EyeSwapchain; input flows through the xr contract as source nodes
(pump_xr_sources deleted); Scene retired.
- pkg.xr_package.in_headset: on-device: editor operable in-headset with the xr executor owning
  the frame loop; `grep pump_xr_sources` empty.
- pkg.xr_package.pose_via_sources: head_pose/controller data reaches the graph only through package
  source nodes (agents and humans share the same path — need.agency).
- pkg.xr_package.view_is_edge: the view is an edge (ADR-037): rewiring the head's view_pose
  input from the head_pose source to a controller pose source is one live
  graph edit — no restart, no native recompile; undo restores it. Scripted
  against simulated pose sources; the on-device run is blessed.

**pkg.render_package** render_region is the GL boundary and the only
place that says GL; draw order is the event chain from render_head.
- pkg.render_package.gl_confined: `grep -rn glDraw components/ | grep -v render_region` is empty.
- pkg.render_package.unwired_no_render: a draw node not wired into the head's chain does not render.
- pkg.render_package.real_pixels: pixels are real: `syg frame` renders a fixture graph headless
  (surfaceless EGL) to raw RGBA and the fixtures/golden-frame.md property
  checks pass — exact clear color, drawn-geometry coverage within bounds,
  color centroid within tolerance, a scripted set_param op between frames
  moves the centroid in the predicted direction, and a draw node outside
  the head's chain contributes zero coverage (pkg.render_package.unwired_no_render in pixels).
- pkg.render_package.shell_is_peer: the shell is the same peer: a windowed host shell is the
  ordinary booted peer with the render executor presenting to a window
  (offscreen surface under test); pointer/key input reaches the graph only
  as package source nodes; a scripted offscreen session drives a pointer
  gesture that adds a node and the next frame's properties change
  accordingly.

**pkg.worker_derivation** The worker region runs derivation-mode
pipelines (exe.derivation_mode) and is placeable cross-peer.
- pkg.worker_derivation.remote_worker: Quest schedules a heavy analysis derivation; with the worker
  capability advertised only by Linux, compilation places it there and the
  result dataset comes back by hash.

**pkg.net_package** Peers exchange advertised node types and provided
datasets; proxies instantiate remote types; net mappings are flavored by inferred discipline (value to coalescable, event to reliable-ordered,
stream to sequenced+jitter).
- pkg.net_package.reconnect_discipline: two-peer test — consumer patch drives a cube from a provider lfo
  through a proxy; kill and reconnect the link; event edges lose nothing,
  value edges resume at latest.

**pkg.placement_as_fallthrough** No placement-specific pass exists;
remote placement emerges from choose-adapters selecting net mappings when the
capability is remote.
- pkg.placement_as_fallthrough.placement_same_passes: the same hello-cosine compiles locally on host (audio present) and
  remotely from the browser (audio remote); diff of engine passes run: none —
  only adapter choice differs.

**pkg.environment_observation** Package splice-in is triggered by
observation nodes (device present, capability advertised), and is visible as
an engine-graph edit event.
- pkg.environment_observation.hotplug_splices: hot-plugging a USB audio device on host splices the audio package
  in without restart; the engine edit appears in the engine graph's history.

## Worked example (test seed)

The spectator scenario end-to-end: browser peer (render + net only) opens
hello-cosine-with-visuals; compilation places audio on host, XR nowhere
(absent everywhere), render locally; assert region placement map, then
pkg.net_package.reconnect_discipline's reconnect discipline, then pkg.environment_observation.hotplug_splices's hot-splice on the host.
