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
2. By-ref true edges within regions + auto `z⁻¹` on cycles
   (preserves certified feedback semantics by construction).
3. `bang`/event rate end-to-end + `queue` mapping; palette/spawner/
   editor edits become event ports; take_edit() and the seq-bump
   pattern retire.
4. Worker region: claude_tmux's system() calls move behind queue
   mappings (today they block the render thread).
5. Audio region: block scheduler + `dac` + latch/snapshot; device
   synths join the graph properly; wav_player's hand-rolled atomics
   retire into ring mappings.
6. Net mappings + advertise-supported-nodes peer exchange (the bridge,
   capabilities-as-nodes).

## Open questions (small, non-blocking)

- Context injection (editor/spawner need graph+registry): ABI
  `set_context` extension as previously sketched; nested-subgraph
  forwarding included. Unchanged by this redesign.
- Worker tier: implied by queue mappings rather than a declared rate —
  revisit if a 4th rate earns its keep.
- text payload rate: currently param-only; becomes event payload when
  string events land (graph edits want this).
