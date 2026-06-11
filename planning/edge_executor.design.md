# Edges, Mappings & Regions — ratified design (Travis + Claude, 2026-06-10)

Supersedes the earlier draft. The conversation that produced this is
summarized in git history; the ontology below is the durable part.
Terminology follows Travis's thesis (see Sygaldry docs: components have
endpoints, throughpoints, parts, plugins).

## Ontology & terminology

- **Endpoint** — an input or output owned by a node. Its type (payload ×
  rate) is declared by the node. `sygaldry_endpoints` already provides
  these (`slider`, `port`, `text`, `toggle`, `bang`).
- **True edge** — a perfectly transparent connection between an output
  and an input. No behavior, no copies: pass-by-reference within a
  region (engineering: references re-resolve on every graph swap, at the
  same point migration runs).
- **Mapping** — a node whose ports are **throughpoints** (sources and
  destinations, not inputs and outputs). A throughpoint's semantic
  domain is *derived from what it is connected to*, not declared.
  Mappings provide behavior at a connection: delay, buffering,
  smoothing, transport. "Mappings are just nodes with throughpoints."
  Implementation note: `PortValue` is a variant, so a dynamic
  throughpoint is a port of kind `any` whose concrete type resolves at
  connect time.
- **Region** — a maximal subgraph that executes synchronously under one
  scheduler: same thread, same cadence, same context. Regions are NEVER
  declared; they are inferred from port types (see rates). Computed as
  connected components of the graph quotiented by rate, recomputed on
  every edit.
- **Peer** — a process or machine. A peer advertises the NODES it
  supports; "I provide `dac` (2 audio-buffer inputs)" *is* the
  capability claim, the permission grant, and the placement constraint.
  There is no separate capability system. Selective advertisement is
  the sandbox model (a browser peer simply doesn't advertise
  shell-shaped nodes). Descriptors already carry name, port schema,
  provenance — the bridge protocol is: exchange advertised descriptors,
  instantiate proxies, flow data through net mappings.

## The type lattice

Every port type = **payload × rate**.

- Payloads (exist today as schema kinds): scalar, bool, vec2, vec3,
  vec4, quat, mat4, texture, draw_call, audio, mesh, text, any.
- Rates:
  - **event** — discrete, must-not-drop, must-not-duplicate (`bang`,
    graph edits, button presses).
  - **frame** — once per video frame; the default for everything today.
  - **block** — once per audio block; hard real-time.
- Some payloads pin a region as well as a rate: texture/draw_call pin
  render (GL context); audio pins the audio thread.

Inference rules:
- A node's region = the strictest rate among its ports
  (block > frame > event).
- A node declaring both audio and draw_call ports is a TYPE ERROR —
  decompose it. (The type system enforces the decomposition ethos.)
- Control (frame) inputs on a block node are snapshotted at block
  boundaries — the standard Max/MSP behavior, via an implicit latch.

## Canonical mappings

Auto-inserted by the executor at inferred boundaries, BUT always visible
in the editor and replaceable by the user (this is where we beat Max:
the boundary is reified as a patchable node, not hidden):

| boundary | mapping | semantics |
|---|---|---|
| cycle (any rate) | `z⁻¹` | unit delay. The certified feedback behavior (one-tick delay) IS this mapping; the old cycle-cut "edge" was never a true edge. |
| frame → block | `latch` | value captured at block boundary |
| block → frame | `snapshot` | last completed block's value |
| event across threads | `queue` | MPSC, never drops (fixes palette-press loss, kills take_edit()) |
| stream across threads | `ring` | SPSC ring buffer (wav_player hand-rolls this today) |
| across peers | `net` | transport flavored by inferred type: value→coalescable, event→reliable-ordered, stream→sequenced+jitter |
| anything → observer | `probe` | /values becomes a mapping |

## Legality

One shared implementation (`components/port_types`) answers, for both
parse time and editor wire time:
- `rate_of(kind)` — payload string → rate
- `connection_legal(from_kind, to_kind)` — equal kinds, or either side
  `any`/`unknown` (subgraph outlets report unknown today)
- `boundary_mapping(from, to)` — which canonical mapping mediates, or
  "" for a true edge, or ILLEGAL (e.g. draw_call → audio)
The editor's wire-drop rule and parse_graph's edge validation both call
this. When a legality question gets smarter, both surfaces inherit it.
(Travis: "everything is a node" applies aspirationally here; it's a
library component until reifying it as a node buys something.)

## Execution model (target)

1. Parse/edit → migrate → type-check edges → infer regions →
   auto-insert boundary mappings (visible) → re-resolve references.
2. Each region ticks its subset at its cadence: render per frame
   (owns GL), audio per block (RT-safe: no alloc/locks/syscalls),
   worker async (may block: STT, tmux, codegen), net at transport pace.
3. True edges within a region are references; mappings carry everything
   across boundaries.
4. One `dac` node per peer owns the audio device stream (advertised =
   available).

## Build order

1. ✅ port_types component: lattice + shared legality (parser + editor).
2. ✅ 2026-06-11: signal_graph_plan — lazily-built TickPlan; true edges are
   cached PortValue* references (resolved once, zero per-tick lookups);
   DFS back edges become DelayMappings (z⁻¹, optional<PortValue> empty
   until first capture = certified tick-1 semantics). Visible as a derived
   "mappings" array in serialize_graph; never persisted.
3. ✅ 2026-06-11 (partial): BangField in the ABI (schema kind "bang",
   event rate in port_types, flows as 0/1 copy with producer-resets
   discipline); event_queue component = the canonical queue mapping;
   take_edit() retired (editor/spawner push into EventQueue via
   set_context; host shell param queue unified on EventQueue too).
   REMAINING: seq-bump retirement blocked on text-payload events (open
   question below); existing button sliders migrate to bang
   opportunistically.
4. ✅ 2026-06-12: components/worker — the peer's worker region (one
   blocking-allowed thread, FIFO, post from any tick). claude_tmux
   rewritten on it: spawn/paste/sleeps off the render thread; results
   return via a shared_ptr atomic block that outlives the instance
   (graph swaps can't dangle queued jobs). The old 240-tick warmup
   hack became a plain 4 s sleep on the worker. Verified live: message
   delivered into a tmux session while the app kept serving frames.
5. ✅ 2026-06-12: components/audio_region — the BLOCK region. Membership
   = dacs + upstream closure through audio edges (a node with audio IN
   but no audio OUT, e.g. spectrogram, stays frame-side by design);
   marked in Graph::offrender so the render plan excludes it. The dac
   node owns the device stream (audio_output: AAudio on Android, SDL2 on
   host AND web — browser plays through Web Audio). Boundaries reified:
   frame→block scalar = LATCH (atomics, applied at block start);
   block→frame scalar = SNAPSHOT; block→frame audio = RING (SPSC,
   republished into Graph::values each render frame so ordinary
   appliers deliver). No device → pump_offline drives blocks from the
   render thread (headless peers keep computing). 4 gtests cover split/
   ring/snapshot/latch; live host demo: lfo→(latch)→osc→(true edge)→dac
   audible, osc→(ring)→spectrogram visible. Device synth migration to
   dac graphs still pending a headset session (kanban
   audio_region_followups).
6. ✅ 2026-06-12 (v1, value-rate): WebSocket transport (http_server /ws
   upgrade + ws_link client + reconnect); peers advertise their node
   types (= capability = permission = placement); POST /peer registers
   each remote type as a proxy descriptor "type@host:port"
   (components/remote_node, SubgraphDescriptor-style slot trampolines).
   Spawning a proxy spawns the real node in the PROVIDER'S live graph;
   inputs forward as coalesced value frames (changed-only); outputs
   mirror back every provider frame (scalar/vec/quat). Proven: two host
   peers, consumer cube driven by provider lfo, params forwarded live.
   REMAINING for full step 6: event-rate reliable-ordered framing,
   stream/jitter, mdns discovery as a source node, plugin shipping over
   the same pipe.

## Open questions (small, non-blocking)

- Context injection (editor/spawner need graph+registry): ABI
  `set_context` extension as previously sketched; nested-subgraph
  forwarding included. Unchanged by this redesign.
- Worker tier: implied by queue mappings rather than a declared rate —
  revisit if a 4th rate earns its keep.
- text payload rate: currently param-only; becomes event payload when
  string events land (graph edits want this).

## Inlets and defaults (ratified, Travis + Claude 2026-06-12)

"Param" is not a category; it is two derived QUALITIES of an inlet:

- **Persistence**: an inlet whose payload has value semantics (scalar,
  bool, text, vec2/3/4, quat) carries a persisted DEFAULT — the value it
  holds when unconnected. The graph JSON `"params"` object is exactly
  the map of these defaults (format unchanged, meaning sharpened).
- **Affordance**: the editor derives a widget from the payload + metadata
  — scalar+range → slider, bool → toggle, text → field, vec3 → triple/
  gizmo, bang → momentary fire button (an affordance with NO persistence,
  bangs have no state). Stream/GPU payloads (audio, texture, draw_call,
  mesh) get a wire handle only.

Semantics:
- An edge into an inlet OVERRIDES its default while connected (not VCV's
  sum — in a graph language, summing is an `add` node, explicitly).
- Editing a connected inlet updates the fallback default; the wire keeps
  ownership of the live value. The editor shows connected inlets as
  METERS (live value), unconnected ones as editable widgets.
- serialize_graph captures DEFAULTS, never edge-driven live values
  (fixes: saving mid-modulation used to persist whatever sample the LFO
  was at).
- Migration re-applies defaults — same machinery, better-stated reason.
- Subgraph inlets are inlets: they take defaults from the node entry's
  params (Pd-abstraction creation args via the existing mechanism) and
  inherit widget metadata from the inner port they forward to.
- Structural creation arguments (port counts, FFT sizes, voice counts)
  are deliberately REJECTED: polyphony is a channel count, variable
  fan-in is wiring, size variants are sibling node types. Dynamic port
  layouts would fight the declarative PFR model at its root.
- text inlets join the wirable world when text-payload edges land (see
  open questions); until then they are persistence-only.

Implementation ladder: kanban/backlog/inlet_defaults.md.
