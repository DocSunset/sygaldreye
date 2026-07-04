# Chapter 7 — Capability packages (PKG)

*A builder implementing this chapter delivers: the package shape (vocabulary +
passes + machinery), the audio package as the reference retrofit, and the XR,
render, worker, store, and net packages on the same shape.*

## Design

An executor flavor is **a world apart** — its own app/engine/execution triple —
spliced into the main engine at runtime when its capability is observed. Three
parts, always:

1. **Vocabulary** — its node types (dac/adc, xr_session, draw, store_*,
   ws/mdns…), present only where linked (SZ-2's manifest is the capability
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

**PKG-1 (package shape).** Every package ships exactly
vocabulary + passes + machinery; passes integrate via fan-ins only
(CMP-3); machinery is reachable only through the package's executor and
contract.
- PKG-1.1: a package can be omitted from a target at build time and the
  system boots with that vocabulary simply absent from the palette.

**PKG-2 (audio retrofit, the reference).** AudioRegion becomes the audio
package with no behavior change.
- PKG-2.1: full existing audio test suite green after the retrofit; region
  membership (dac closure), latch/snapshot/ring semantics, and pump_offline
  identical (EXE-2, EXE-4, EXE-6.2 re-run unchanged).

**PKG-3 (xr package).** Absorbs xr.cpp + XrSessionObj + FrameLoop + Renderer/
EglContext/EyeSwapchain; input flows through the xr contract as source nodes
(pump_xr_sources deleted); Scene retired.
- PKG-3.1: on-device: editor operable in-headset with the xr executor owning
  the frame loop; `grep pump_xr_sources` empty.
- PKG-3.2: head_pose/controller data reaches the graph only through package
  source nodes (agents and humans share the same path — N4).

**PKG-4 (render package).** render_region is the GL boundary and the only
place that says GL; draw order is the event chain from render_head.
- PKG-4.1: `grep -rn glDraw components/ | grep -v render_region` is empty.
- PKG-4.2: a draw node not wired into the head's chain does not render.

**PKG-5 (worker/derivation).** The worker region runs derivation-mode
pipelines (EXE-7) and is placeable cross-peer.
- PKG-5.1: Quest schedules a heavy analysis derivation; with the worker
  capability advertised only by Linux, compilation places it there and the
  result dataset comes back by hash.

**PKG-6 (net package).** Peers exchange advertised node types and provided
datasets; proxies instantiate remote types; net mappings are flavored by
inferred type (value→coalescable, event→reliable-ordered,
stream→sequenced+jitter).
- PKG-6.1: two-peer test — consumer patch drives a cube from a provider lfo
  through a proxy; kill and reconnect the link; event edges lose nothing,
  value edges resume at latest.

**PKG-7 (placement as fallthrough).** No placement-specific pass exists;
remote placement emerges from choose-adapters selecting net mappings when the
capability is remote.
- PKG-7.1: the same hello-cosine compiles locally on host (audio present) and
  remotely from the browser (audio remote); diff of engine passes run: none —
  only adapter choice differs.

**PKG-8 (environment observation).** Package splice-in is triggered by
observation nodes (device present, capability advertised), and is visible as
an engine-graph edit event.
- PKG-8.1: hot-plugging a USB audio device on host splices the audio package
  in without restart; the engine edit appears in the engine graph's history.

## Worked example (test seed)

The spectator scenario end-to-end: browser peer (render + net only) opens
hello-cosine-with-visuals; compilation places audio on host, XR nowhere
(absent everywhere), render locally; assert region placement map, then
PKG-6.1's reconnect discipline, then PKG-8.1's hot-splice on the host.
