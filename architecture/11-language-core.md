# Chapter 11 — The language core (LNG)

*The graph system as a language: its value vocabulary, its edit vocabulary,
its composition mechanism, and its interchange form. Everything here is part
of the language itself, not any application of it. A builder implementing
this chapter delivers: the payload/kind catalog, span semantics, the event
discipline, the inlet model, structured edit ops, subgraphs, and the
serialization round-trip.*

## Design

**The value vocabulary (payload kinds).** One kind system (nam.one_kind_system) spans wires
and store. The fiat payload kinds: `scalar, bool, vec2, vec3, vec4, quat,
mat4, text, bang, audio, texture, draw_call, mesh, surface, span, any,
graph, ops, cidset` (the last three joined with ADR-034's structured
lane: graphs, edit-op batches, and cid sets as first-class payloads). Each
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
DEFAULT — the value it holds when unconnected; the defaults node (sto.composite_graphs) is
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
editing (cmp.projection_editing) all emit the same vocabulary, toward each live instance's one
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

**lng.kind_catalog** The fiat payload kinds exist as kind nodes in the
one kind system; constraints (rate pins, region pins) live on the kind node.
- lng.kind_catalog.catalog_enumerable: enumerating the kind registry yields the catalog above; each
  entry resolves to decoders + constraints (no hardcoded switch elsewhere —
  the legality oracle reads kind nodes).

**lng.span_semantics** cell_rank derives from kind; excess-rank edges
lift; span to cell and cell to span are legal; whole-by-kind never lifts.
- lng.span_semantics.span_stamps_clones: N-by-3 span to vec3 input stamps N clones; N=1 degenerates to one;
  resize preserves clone state by key (exe.migration.reorder_preserves_state).
- lng.span_semantics.span_whole_one_draw: N-by-3 span to color_mesh instance input draws N instances in ONE
  call (no clones — the draw boundary consumes the span whole).

**lng.event_discipline** Events never drop or duplicate across any
legal path; bangs carry no state.
- lng.event_discipline.bangs_exact_stateless: 10,000 bangs through [button to queue to counter across threads]
  count exactly 10,000; serialize of the patch contains no bang state.

**lng.inlet_model** Defaults persist; edges override; connected inlets
meter; affordances derive from kind + metadata.
- lng.inlet_model.default_not_live: exe.plan_cache.saves_default / edr.defaults_discipline.default_is_fallback (defaults never capture live modulation).
- lng.inlet_model.widget_data_driven: the widget table is data-driven: adding range metadata to a
  scalar inlet switches its widget without editor code changes.

**lng.edit_ops** One structured vocabulary for all mutation; ops are
serializable and attributable; whole-graph replace routes through it.
- lng.edit_ops.replay_reproduces: recording the ops of an editor session and replaying them onto
  the session's initial graph reproduces the final topology hash.
- lng.edit_ops.op_attributed: an op arriving over the mesh carries its author peer key
  (attribution for MSH audit and documentary provenance).

**lng.subgraphs** Templates clone behind trampolines; defaults-as-
creation-args; resource-holder inference; graph datasets register as node
types.
- lng.subgraphs.dataset_is_node_type: `card.json` in graphs_dir appears in the palette; spawning it
  clones the template; its inner position inlet takes the node entry's
  param as default.
- lng.subgraphs.resource_holder_refuses: a subgraph containing dac is refused lifting with the
  resource-holder error (message names the inner culprit).

**lng.context_seam** No node reaches a global; context arrives by
injection and forwards through nested subgraphs.
- lng.context_seam.context_forwards: static audit (exe.executor_contract.no_singleton_reach's grep) plus: a subgraph two levels deep
  containing graph_source still publishes the root graph's keys.

**lng.round_trip** parse -after- serialize = identity on the persisted
surface; derived structure never persists.
- lng.round_trip.roundtrip_no_derived: property test over the graphs_dir corpus: serialize(parse(j))
  is JSON-equal to j modulo key order; no `mappings` key in any persisted
  file.

**lng.text_events** Design the text event payload; until
ratified, text inlets are persistence-only. (ADR-034 unblocked graph-edit
events separately: ops ride the structured lane as their own kind —
lng.structured_payloads.op_event_applies. This requirement now gates only text semantics and the seq-bump
retirement.)

**lng.query_vocabulary** traverse, filter, join,
fixpoint are core node types; queries run as committed derivations (memoized index datasets) or as standing reactive values (incrementally maintained under
ADR-015).
- lng.query_vocabulary.lineage_query: "takes whose lineage includes CID_osc-type" wired from the four
  primitives returns the expected set over a seeded store corpus; re-running
  is a memo hit.
- lng.query_vocabulary.standing_incremental: the standing form updates within one demand cycle of a new
  qualifying commit, and recomputes only its dirty cone (counter).
- lng.query_vocabulary.fixpoint_terminates: fixpoint over a cyclic link structure terminates (visited-set
  semantics) — no query can hang the store.
- lng.query_vocabulary.palette_is_query: the palette and the back-link index are expressed as queries
  (no bespoke search code paths).

**lng.structured_payloads** The node contract carries
kind-tagged structured values (graph, edit ops, text, the catalog's
non-float kinds) on the event and value disciplines: declared in endpoints
structs like any port, codec and accessors generated (abi.one_declaration), legality by
the one promise oracle, zero-copy in-process, canonical encoding only at
commit boundaries. The float/stream path is unchanged; the oracle refuses
structured payloads on stream.
- lng.structured_payloads.graph_value_zero_copy: a graph value flows over an edge between two realized instances
  (the engine's receive0 → regions0 hop); the consumer reads it through
  generated accessors; no serialization on the hop.
- lng.structured_payloads.float_path_free: the RT audits (exe.realtime_safety.zero_alloc_lock, nam.promise_oracle.no_lookup_at_tick) and golden audio re-run green
  after the widening — the float path pays nothing.
- lng.structured_payloads.op_event_applies: a node emits an edit op as an event payload; wired into another
  graph's arbiter inlet, the op applies — a graph edits a graph through
  wiring alone, no privileged surface (the lng.text_events unblock for op events).
- lng.structured_payloads.query_evaluator_dissolves: the query four run as realized instances exchanging set values
  over the structured lane; the bespoke query evaluator dissolves
  (scaffolding retired; lng.query_vocabulary's criteria stay green through the swap).

## Worked example (test seed)

A polyphonic hello-cosine without new machinery: wire an 8-by-1 span of
frequencies into osc0/freq — lng.span_semantics.span_stamps_clones stamps 8 clones keyed by index; their
audio mixes (an explicit `mix` node) into vca0. Save/reload (lng.round_trip.roundtrip_no_derived),
replay the session's edit ops (lng.edit_ops.replay_reproduces), and confirm the palette spawned it
all from a `poly8.json` subgraph template (lng.subgraphs.dataset_is_node_type).
