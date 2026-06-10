# Edge & Executor Design (DRAFT — for Travis to ratify)

_Drafted 2026-06-10 from evidence gathered during the decomposition slice.
Nothing here is implemented beyond what's marked EXISTS._

## What an edge is today (EXISTS)

One kind: **latest-value copy at tick time**. `tick_graph` walks nodes in
topo order; for each node it copies every in-edge's source value from the
`values` map into the target input port, then calls `process`. All outputs
land in `values` after process. Single thread, one tick per video frame.

This is a *pull-less, push-less, frame-synchronous data flow* — a good
match for control signals and per-frame render data, and it is what every
demo so far runs on.

## Evidence: what flowed through edges this session

| Payload | Where seen | Semantics actually needed |
|---|---|---|
| scalars (lfo→cube.scale, slider→smooth) | everywhere | latest value; loss OK |
| vec3/quat (hand→editor, sun→cube) | interaction rig | latest value; loss OK |
| mat4 (camera.pv → platform draw) | spine | latest value, same frame |
| DrawFn closures | every visual node | collected per tick, ordered |
| GpuTexture handles (rd sim→renderer) | RD split | latest value **+ implied GPU ordering** (same thread/context makes it safe today) |
| AudioBuffer (borrowed ptr) | Android synths | **valid only during tick** — a different lifetime contract hiding inside the same edge kind |
| button.pressed → spawner.trigger | graph-built palette | **event**: rising edge mattered; we hand-rolled edge detection in the consumer. A dropped frame = dropped press. |
| graph-edit JSON | editor/spawner → platform | **out-of-band** (take_edit): not an edge at all because strings can't flow |

## Proposed edge taxonomy

1. **value** (exists): latest-value copy. Default. Loss OK, frame-aligned.
2. **event**: discrete, must-not-drop, must-not-duplicate. Carries a
   payload (initially: float, string). Delivered as a *queue drained per
   tick* on the consumer side. Rising-edge detection moves out of node
   code (`trigger_edge`, spawner's `prev_on_`) into the edge itself.
3. **stream**: ordered chunks at a producer-defined rate (audio blocks).
   Today's AudioBuffer borrowed-pointer contract becomes explicit: a
   stream edge owns a ring buffer between regions.
4. **resource**: handle + ordering dependency (GpuTexture). Same as value
   while producer/consumer share a GL context+thread; becomes a sync
   point (fence) when regions split.

Recommendation: implement **event** next (it unblocks string edges, makes
the palette robust, and is cheap); leave stream/resource as taxonomy until
the audio region exists.

## Execution regions

A region = (thread, cadence, context). Proposed:
- **render**: per video frame, owns GL. Today's only region.
- **audio**: per audio block (e.g. 256 frames @ 48k), no GL, hard
  deadline. Synth/filter/output nodes live here.
- **net**: wall-clock/async; sockets live here so blocking I/O never
  touches render (today udp nodes do nonblocking I/O on render — fine,
  but a real bridge with TCP/WebSocket cannot).

Intra-region edges stay direct (current mechanism). Cross-region edges are
queues with kind-specific policy: value = triple-buffer/latest, event =
MPSC queue, stream = ring buffer. Node→region assignment: by node-type
metadata (descriptor field), overridable per instance in graph JSON.

The graph stays ONE graph; regions are an executor concern. tick_graph
becomes per-region: each region ticks the subset of nodes assigned to it,
edges within the subset direct, edges crossing get queue endpoints.

## Context injection (the nested-subgraph problem)

editor/spawner need (graph, registry); platform pumps them by type-name
special case, and nodes inside subgraphs never get context. Options:

A. **Context struct on the descriptor**: extend ABI with optional
   `set_context(void*, const EyeballsContext*)` where EyeballsContext is
   a versioned struct {graph, registry, region, ...}. tick_graph calls it
   for every node each tick (cheap null-check); SubgraphNode forwards to
   inner ticks. Kills the host_app special cases AND fixes nesting.
B. Context as ports (graph/registry as PortValue) — rejected: the graph
   contains the node; circular ownership and serialization nonsense.
C. Global/thread-local context — rejected: invisible coupling, breaks
   multiple instances.

Recommendation: **A** — it is mechanical, ABI-versioned, and SubgraphNode
forwarding is one line.

Graph edits as events: with event edges carrying strings, editor/spawner
emit `graph_edit` events on an output port; a `graph_sink` platform node
consumes them. take_edit() dies; edits become visible, patchable wiring.

## String params vs string edges

Params (EXISTS): text<> fields. Edges: only via the **event** kind above —
strings as steady-state per-frame values invite allocation churn and
"who owns this" questions; as events they have clear lifecycle.

## Open questions for Travis

1. Edge-kind syntax in JSON: `{"from":..,"to":..,"kind":"event"}` with
   "value" default — OK?
2. Region assignment: per node-type default + per-instance override —
   OK? Names: render/audio/net?
3. Event delivery order vs topo order: drain before process (same tick if
   producer earlier in topo, else next tick) — acceptable nondeterminism,
   or do events always defer one tick for uniformity?
4. Context struct contents v1: {graph*, registry*} — anything else now
   (time source? region id? rng?)
5. Is `graph_sink` the right shape for self-edit, or should the executor
   itself consume graph_edit events without a node?
