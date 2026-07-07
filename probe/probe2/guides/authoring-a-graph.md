# Authoring a graph

A graph is the default way to make anything in sygaldreye. This is where you
should start for every new behaviour (L22). If you find you *can't* express it
as a graph yet, that's the bug — go build the missing leaf as a native (see
`authoring-a-native.md`), then come back and compose it.

## What a graph is

A graph is a dataset: nodes (data as containers), links (data as references),
and the defaults that make it concrete. The canonical example, `graphs/hello-
cosine.json`:

```json
{
  "kind": "graph",
  "lock": {},
  "topology": {
    "nodes": {
      "osc0": {"type": "osc"},
      "lfo0": {"type": "lfo"},
      "vca0": {"type": "vca"},
      "dac0": {"type": "dac"}
    },
    "edges": [
      {"from": "osc0/out", "to": "vca0/in"},
      {"from": "lfo0/out", "to": "vca0/gain"},
      {"from": "vca0/out", "to": "dac0/in"}
    ]
  },
  "defaults": {
    "osc0/freq": 220.0,
    "osc0/shape": "cosine",
    "lfo0/freq": 0.5
  }
}
```

The pieces:

- **`topology.nodes`** — `id → {type}`. The `type` is a name; where it resolves
  is the palette's business (native, subgraph, frozen, or plugin — all four look
  the same here).
- **`topology.edges`** — `from "id/port" → to "id/port"`. An edge is legal iff
  the connection oracle accepts the two ports' **(kind, discipline)** promises.
  A cosine `out` (audio/block) into a `vca/in` (audio/block) is legal; a
  frame-discipline value into a block-clocked consumer is legal too, but the
  compiler will insert a **mapping** at that boundary (a latch) — visible and
  replaceable, never silent (see Projection editing, below).
- **`defaults`** — `route → value`. **Params ARE the doc** (L13): editing a
  parameter writes the default here, not some side table. A connected inlet
  (like `vca0/gain`, fed by the LFO) still keeps its default as the *fallback*;
  serialization captures the default, never a live sample.
- **`lock`** — `name → type CID` (ADR-026). Empty while you're authoring; filled
  when the graph is committed, pinning each type name to an exact version. A
  vocabulary upgrade is one lock swap — it rewrites the lock, never the
  topology, so the topology hash and its provenance chain are untouched.

## Regions and mappings (what the compiler does for you)

You never declare regions. The executor infers them from the ports' promises:
the **block region** is the sink-closure through block-discipline stream edges
(everything feeding the dac at the block clock); the **frame region** is the
value cells; anything demanded by neither and not clocked is **inert** (an
honest lint, not an error). Boundary crossings get mappings from the one promise
oracle — a `latch` when a value source feeds a clocked consumer, a `snapshot`
when the frame side reads the last completed block. These are derived, never
persisted, and always replaceable by an explicit mapping node (a `smoother` in
place of a latch, say). Open the **realized view** to see them.

## Making a reusable graph: subgraphs

A `.json` in `graphs/` **is** a node type (LNG-6: the graphs dir is the
palette). Give it `inlets` and `outlets` and it composes like any native:

```json
{
  "kind": "graph",
  "topology": {
    "nodes": {"t0": {"type": "osc"}},
    "edges": [],
    "inlets":  {"freq": "t0/freq", "shape": "t0/shape"},
    "outlets": {"out": "t0/out"}
  },
  "defaults": {}
}
```

Now `{"type": "osc_sub"}` works anywhere `{"type": "osc"}` does. Two more knobs:

- **creation args** — a subgraph instance can take params (`{"freq": 880}`) that
  set its inner defaults.
- **`lift_key`** — keyed lifting: a span of N values fans a subgraph into N
  clones keyed by a stable id (the editor's cards are a subgraph lifted by node
  id). The clone's state migrates by route, so live re-ordering is byte-
  identical to an uninterrupted render.

## The four routes, one registry

The same patch renders identically whether `osc0`'s type resolves to:

1. a **native** (`osc` in C++),
2. a **subgraph** (`osc_sub.json`),
3. a **frozen artifact** (a `.so` the freezer emitted), or
4. a **shipped plugin** (an out-of-tree `.so` through the trust gate).

They're palette-identical through `registry_face`. This is what lets you
prototype as a graph and later freeze or ship a leaf without touching the patch.

## Running it

- `syg render-graph <seconds>` — interchange JSON on stdin → executor semantics
  (regions, latches, quiescence) → raw float32 mono. This is the golden-audio
  path.
- `syg edit` — open the graph live and gesture on it (see
  `live-edits-as-an-llm-peer.md`).
- `syg peer` — one booted peer holding the graph as its app, with a resident
  engine level you can compile and edit.

## Freezing (when a graph is too slow)

Performance alone never justifies a native (ADR-033). Instead, freeze the graph:
codegen is realize's second backend, selected by wiring a `text_cell` reading
`"codegen"` into the compiler's `backend` port. The artifact is self-contained
(the kernel math verbatim, a baked delay line, `extern "C" float sinf(float)` —
it links `-ffreestanding` on bare metal) and byte-identical to the interpreted
render. It's also a plugin: the same `.so` exposes the movement ABI for hot-swap
and the node-type ABI for the registry. Unfreezing is just reading the store —
the provenance walk recovers the source graph, the recipe, and the app.

A frozen movement is **crownless**: it renders (movement-level conformance) but
has no wire surface (it fails peer-level, by design). That's the seam between a
sealed firmware and a mesh citizen.

## Discipline

- **Everything round-trips.** If you hand-write bytes or serialize outside the
  generated codec, you've failed FMT-1 even if no test caught you yet.
- **Defaults, not live values.** Serialization captures the persisted surface.
- **Blessing is testimony.** Golden expected output (does this *sound* right?)
  enters by capture — a human or a delegated agent once said "this is right,"
  signed and dated. You don't self-bless.
