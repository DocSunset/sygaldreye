# Chapter 9 — Editor, documents, and the store browser (EDR)

*A builder implementing this chapter delivers: the graph editor (already
decomposed into nodes in the codebase) held to its laws, projection-editing
UX, the store browser on the astui navigation model, and the document layer's
validation targets.*

## Design

**The editor is graphs editing graphs.** Cards, wires, gestures, palette,
labels are node compositions (shipped 2026-06-15: graph_source to lifted card
subgraph keyed by id to draw chain; gesture nodes emitting structured edit
ops). Inlets carry two derived qualities — persistence (a default, in the
defaults node) and affordance (widget derived from kind + metadata; connected
inlets render as meters, unconnected as widgets). Editing a connected inlet
updates the fallback default; serialization captures defaults, never live
values. Undo snapshots on structural change; param drift never trashes
history.

**Projection editing UX (cmp.projection_editing surfaced).** The editor can open any level:
app graph, execution graph (realized view), engine graph. In realized views,
compiler-introduced structure (mappings, adapters, placement) is visible;
edits write back through the inverse compilation map; a write-back conflict
is presented as such (rebase semantics); "make this a pass" and "fork this"
are explicit, deliberate gestures — never defaults.

**The store browser is the astui walker.** here (a single node), path
(breadcrumb history, not a menu), frontier (everything reachable from here,
recomputed each step, labeled by outward-reading edge), mark (the authoring
primitive: the marked subgraph + hand-authored links = the durable map, a
dataset like any other). Movement chooses a destination, not a direction.
The ground is the synthetic node connected to all nodes; walks begin there.

**Documents.** A document is a graph; transclusion is an address-valued edge
(fixed = quotation, live = subscription); a document's proposed render
program is a declinable graph run in the reader's advertised vocabulary
(msh.three_lists). Bidirectional links: attach to hashes, derive per-version positions
through the version map (nam.sequence_traversal), index back-links per store (sto.back_link_index).
Validation targets inherited from the rhizome probe: the **round-trip
metric** (C++  and back  document form, byte-identical) and the **trace** (a user's or
agent's remixed transcluded path through a corpus, itself a committable
dataset).

**Agents are peers, not backdoors.** Agent look/move/click/edit flows through
the same source nodes as human input; every bug an agent can reproduce, a
human can, and vice versa.

## Open directions (inherited from the rhizome probe, undesigned)

Authoring hand-made connections (a *focus* notion versus connecting among
marked lines), embedding vectors — LLM-derived geometry for
similarity-based movement (promising, not committed), git-repo frontend
and a read-only FUSE adapter as free inspection surfaces (rendering/
controller adapter directions), the agent trace as a first-class shareable
dataset (the walker's path, committed — partially covered by the worked
example below).

## Requirements

**edr.editor_is_nodes** No editor monolith returns; all editor behavior
is node compositions replaceable at runtime.
- edr.editor_is_nodes.palette_swap_live: replacing the palette subgraph live (swap+migrate) changes editor
  behavior without restart.
- edr.editor_is_nodes.surface_is_graphs: the surface is graphs: every editor-surface behavior — cards,
  wires, labels, palette, layout, gesture logic — lives in
  graphs/editor/*.json over the render/text leaf vocabulary; the set of
  registered natives exactly equals vocabulary/packages.json, and no
  source file under src/ implements an editor-surface concept (gate grep:
  card/palette/wire_drag/slider_drag/dwell/gesture).

**edr.defaults_discipline** Serialize captures defaults; connected
inlets are meters; editing one updates the fallback.
- edr.defaults_discipline.default_is_fallback: modulate vca0/gain with lfo0, save, reload: gain default is the
  edited fallback, not an LFO sample (exe.plan_cache.saves_default at the UX level).

**edr.undo** Structural-snapshot undo; ref-trail undo for commits; the
two compose.
- edr.undo.undo_sequence_restores: sequence [add node, drag param, remove node, undo, undo] restores
  the original topology and defaults exactly.

**edr.realized_view_editing** The latch of hello-cosine is visible,
labeled as compiler-inserted, replaceable; conflict and fork flows per
cmp.projection_editing, cmp.fork have explicit UI.
- edr.realized_view_editing.gesture_replaces_latch: scripted gesture test drives the smoother replacement of cmp.projection_editing.writeback_smoother
  through the editor's own gesture nodes (agent-driven, per edr.agents_as_peers).

**edr.store_browser** here/path/frontier/mark over any store graph;
frontier latency stays interactive (demand-driven, memoized).
- edr.store_browser.walk_and_mark: walk ground to graphs/hello-cosine to topology to osc0 to type osc  to 
  ports; mark osc0; the marked map persists as a dataset and re-opens.
- edr.store_browser.frontier_paginates: frontier of a node with 100,000 links paginates without blocking the
  frame region.

**edr.documents** Transclusion renders live and fixed correctly;
declining a proposed renderer falls back to the reader's kind decoder.
- edr.documents.live_vs_fixed_transclusion: a document transcluding `graphs/hello-cosine:nodes/osc0/freq`
  (live) updates when the ref moves; the same address normalized (fixed)
  does not.
- edr.documents.cpp_roundtrip: round-trip metric harness: decompose a small permissive C++ file
  to document form and regenerate byte-identically (modulo clang-format);
  wire as a CI property test over a corpus directory.

**edr.agents_as_peers** Every editor gesture is drivable through source
nodes over the mesh; no privileged agent API exists.
- edr.agents_as_peers.human_agent_identical: the full editor integration suite runs twice — human-input
  simulation and agent-source driving — with identical resulting graphs.
- edr.agents_as_peers.author_from_scratch: authoring from scratch: a scripted gesture session in the host
  shell, starting from an EMPTY graph, authors hello-cosine end-to-end
  through pointer-source-driven gesture ops (spawn from palette, wire,
  set params, save); the saved doc's topology matches the fixture modulo
  generated ids, and its render passes the golden-audio properties.

**edr.observability** Pull-observability (values snapshot, probes) and
purpose-built engine-graph probes; audio inspection emits
spectrogram/waveform + numeric features (agents can't hear).
- edr.observability.probe_any_edge: a probe mapping attached to any edge exposes its value stream
  over the values surface without altering region inference.

## Worked example (test seed)

An agent peer, using only advertised source nodes: opens the editor, walks
the store browser to hello-cosine, patches noise0 through a new vca, replaces
the auto latch with smoother (edr.realized_view_editing.gesture_replaces_latch), commits (ref moves), records a take,
marks its path through the store as a trace dataset — the medium's whole
gesture set exercised in one scripted, assertable run.
