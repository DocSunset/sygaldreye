# Chapter 11 — The language core (LNG)

*The graph system as a language: its value vocabulary, its edit vocabulary,
its composition mechanism, and its interchange form. Everything here is part
of the language itself, not any application of it. A builder implementing
this chapter delivers: the payload/kind catalog, span semantics, the event
discipline, the inlet model, structured edit ops, subgraphs, and the
serialization round-trip.*

## Design

**The value vocabulary (payload kinds).** One kind system (NAM-4) spans wires
and store. The fiat payload kinds: `scalar, bool, vec2, vec3, vec4, quat,
mat4, text, bang, audio, texture, draw_call, mesh, surface, span, any`. Each
is a kind node carrying decoders, affordance metadata, and constraints (audio
pins block rate; texture/draw_call pin the render region). `any` and
`unknown` are the honest gradual-typing escape hatches (subgraph outlets
report unknown until resolved).

**Spans and rank.** A `span` is an N-by-cell array with a named cell kind.
Rank is derived from kind (`cell_rank`); **excess rank drives lifting**: a
span producer wired to a cell-kind consumer stamps N clones (EXE/AUT), and a
span into a draw node's per-instance input becomes one instanced draw.
`whole<T>` is the opt-out for a future consumer that takes an N-by-cell array as
a unit (reduce/gather/flocking); today no node needs it — whole-by-kind falls
out for audio/mesh/texture/etc. Axis discipline: channels map, frames scan;
named axes are the planned disambiguation (deferred work item).

**Events and bangs.** `event` rate = discrete, must-not-drop, must-not-
duplicate. `bang` is the stateless event payload (affordance: momentary fire;
no persistence — bangs have no state). Discipline: producer resets after the
appliers run; events cross threads only through the `queue` mapping (MPSC,
never drops); within a region, appliers run before process() so a same-tick
linear chain is a legal ordering declaration. OPEN (carried from the edge
design): text-payload events — text joins the wirable world when string
events land; graph edits want this; until then text inlets are
persistence-only.

**Inlets: persistence and affordance (two derived qualities, not a "param"
category).** An inlet whose payload has value semantics carries a persisted
DEFAULT — the value it holds when unconnected; the defaults node (STO-7) is
exactly the map of these. The editor derives a WIDGET from payload + metadata
(scalar+range to slider, bool to toggle, text to field, vec3 to gizmo, bang  to 
momentary; stream/GPU payloads get a wire handle only). An edge into an inlet
OVERRIDES its default while connected (summing is an `add` node, explicitly —
not VCV's implicit sum); editing a connected inlet updates the fallback;
connected inlets render as meters. Structural creation arguments (port
counts, FFT sizes, voice counts) are deliberately REJECTED: polyphony is a
channel count, variable fan-in is wiring, size variants are sibling node
types — dynamic port layouts would fight the declarative reflection model at
its root.

**The edit vocabulary.** All mutation flows through structured edit ops:
`add_node, remove_node, add_edge, remove_edge, set_param` (and whole-graph
replace as the degenerate case). Ops are data — queueable, serializable,
attributable, replayable; they carry INVERSES and PRECONDITIONS (ADR-018 and 023);
gestures are transactions (coalescing brackets); history is the op tree,
linearity a view; the editor's gestures, remote peers, agents, and projection
editing (CMP-4) all emit the same vocabulary, toward each live instance's one
arbiter queue. The boot tape is these ops in fixed-format records (ch. 14).

**Subgraphs (composition).** A subgraph node clones a template graph behind
slot trampolines: inner inlets/outlets forward through the node's ports;
inner inlets take their defaults from the node entry's params (Pd-abstraction
creation args, without structural args) and inherit widget metadata from the
inner port they forward to. Subgraph definitions are datasets; the registry
loads graph datasets as node types exactly like natives (a `.json` in
graphs_dir IS a plugin). A subgraph node whose inner nodes include a resource
holder is itself a resource holder (inferred, not declared). Lift keys let a
lifted subgraph key its clones on data identity (`card` keyed by node id).

**Context seams.** Resource holders receive their context (graph handle, edit
queue, shared overrides, registry) by injection at pump time — never by
global reach. The seam is the ABI's set_context; nested subgraphs forward it.
This is the one sanctioned way a node sees "the graph it lives in"
(graph_source reifies the enclosing graph as wired outputs through exactly
this seam).

**Interchange form.** The composite graph (topology + defaults) serializes as
the single-file graph JSON: `nodes` (id to type + params), `edges`, optional
`lift_key`, subgraph inlets/outlets. Serialization captures DEFAULTS, never
edge-driven live values; derived structure (auto-inserted mappings, regions,
islands) appears only in derived output (serialize shows a `mappings` array,
never persisted). parse -after- serialize = identity on the persisted surface.

## Requirements

**LNG-1 (kind catalog).** The fiat payload kinds exist as kind nodes in the
one kind system; constraints (rate pins, region pins) live on the kind node.
- LNG-1.1: enumerating the kind registry yields the catalog above; each
  entry resolves to decoders + constraints (no hardcoded switch elsewhere —
  the legality oracle reads kind nodes).

**LNG-2 (span semantics).** cell_rank derives from kind; excess-rank edges
lift; span to cell and cell to span are legal; whole-by-kind never lifts.
- LNG-2.1: N-by-3 span to vec3 input stamps N clones; N=1 degenerates to one;
  resize preserves clone state by key (EXE-5.2).
- LNG-2.2: N-by-3 span to color_mesh instance input draws N instances in ONE
  call (no clones — the draw boundary consumes the span whole).

**LNG-3 (event discipline).** Events never drop or duplicate across any
legal path; bangs carry no state.
- LNG-3.1: 10,000 bangs through [button to queue to counter across threads]
  count exactly 10,000; serialize of the patch contains no bang state.

**LNG-4 (inlet model).** Defaults persist; edges override; connected inlets
meter; affordances derive from kind + metadata.
- LNG-4.1: EXE-1.2 / EDR-2.1 (defaults never capture live modulation).
- LNG-4.2: the widget table is data-driven: adding range metadata to a
  scalar inlet switches its widget without editor code changes.

**LNG-5 (edit ops).** One structured vocabulary for all mutation; ops are
serializable and attributable; whole-graph replace routes through it.
- LNG-5.1: recording the ops of an editor session and replaying them onto
  the session's initial graph reproduces the final topology hash.
- LNG-5.2: an op arriving over the mesh carries its author peer key
  (attribution for MSH audit and documentary provenance).

**LNG-6 (subgraphs).** Templates clone behind trampolines; defaults-as-
creation-args; resource-holder inference; graph datasets register as node
types.
- LNG-6.1: `card.json` in graphs_dir appears in the palette; spawning it
  clones the template; its inner position inlet takes the node entry's
  param as default.
- LNG-6.2: a subgraph containing dac is refused lifting with the
  resource-holder error (message names the inner culprit).

**LNG-7 (context seam).** No node reaches a global; context arrives by
injection and forwards through nested subgraphs.
- LNG-7.1: static audit (EXE-6.1's grep) plus: a subgraph two levels deep
  containing graph_source still publishes the root graph's keys.

**LNG-8 (round-trip).** parse -after- serialize = identity on the persisted
surface; derived structure never persists.
- LNG-8.1: property test over the graphs_dir corpus: serialize(parse(j))
  is JSON-equal to j modulo key order; no `mappings` key in any persisted
  file.

**LNG-9 (text events — OPEN).** Design the text event payload so graph edits
can flow as events; until ratified, text inlets are persistence-only and
this requirement gates the seq-bump retirement.

**LNG-10 (query vocabulary — core, ADR-024).** traverse, filter, join,
fixpoint are core node types; queries run as committed derivations (memoized index datasets) or as standing reactive values (incrementally maintained under
ADR-015).
- LNG-10.1: "takes whose lineage includes CID_osc-type" wired from the four
  primitives returns the expected set over a seeded store corpus; re-running
  is a memo hit.
- LNG-10.2: the standing form updates within one demand cycle of a new
  qualifying commit, and recomputes only its dirty cone (counter).
- LNG-10.3: fixpoint over a cyclic link structure terminates (visited-set
  semantics) — no query can hang the store.
- LNG-10.4: the palette and the back-link index are expressed as queries
  (no bespoke search code paths).

## Worked example (test seed)

A polyphonic hello-cosine without new machinery: wire an 8-by-1 span of
frequencies into osc0/freq — LNG-2.1 stamps 8 clones keyed by index; their
audio mixes (an explicit `mix` node) into vca0. Save/reload (LNG-8.1),
replay the session's edit ops (LNG-5.1), and confirm the palette spawned it
all from a `poly8.json` subgraph template (LNG-6.1).
