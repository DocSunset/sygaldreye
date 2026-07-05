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

**Projection editing UX (CMP-4 surfaced).** The editor can open any level:
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
(MSH-3). Bidirectional links: attach to hashes, derive per-version positions
through the version map (NAM-7), index back-links per store (STO-9).
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

**EDR-1 (editor is nodes).** No editor monolith returns; all editor behavior
is node compositions replaceable at runtime.
- EDR-1.1: replacing the palette subgraph live (swap+migrate) changes editor
  behavior without restart.

**EDR-2 (defaults discipline).** Serialize captures defaults; connected
inlets are meters; editing one updates the fallback.
- EDR-2.1: modulate vca0/gain with lfo0, save, reload: gain default is the
  edited fallback, not an LFO sample (EXE-1.2 at the UX level).

**EDR-3 (undo).** Structural-snapshot undo; ref-trail undo for commits; the
two compose.
- EDR-3.1: sequence [add node, drag param, remove node, undo, undo] restores
  the original topology and defaults exactly.

**EDR-4 (realized-view editing).** The latch of hello-cosine is visible,
labeled as compiler-inserted, replaceable; conflict and fork flows per
CMP-4/5 have explicit UI.
- EDR-4.1: scripted gesture test drives the smoother replacement of CMP-4.1
  through the editor's own gesture nodes (agent-driven, per EDR-7).

**EDR-5 (store browser).** here/path/frontier/mark over any store graph;
frontier latency stays interactive (demand-driven, memoized).
- EDR-5.1: walk ground to graphs/hello-cosine to topology to osc0 to type osc  to 
  ports; mark osc0; the marked map persists as a dataset and re-opens.
- EDR-5.2: frontier of a node with 100,000 links paginates without blocking the
  frame region.

**EDR-6 (documents).** Transclusion renders live and fixed correctly;
declining a proposed renderer falls back to the reader's kind decoder.
- EDR-6.1: a document transcluding `graphs/hello-cosine:nodes/osc0/freq`
  (live) updates when the ref moves; the same address normalized (fixed)
  does not.
- EDR-6.2: round-trip metric harness: decompose a small permissive C++ file
  to document form and regenerate byte-identically (modulo clang-format);
  wire as a CI property test over a corpus directory.

**EDR-7 (agents as peers).** Every editor gesture is drivable through source
nodes over the mesh; no privileged agent API exists.
- EDR-7.1: the full editor integration suite runs twice — human-input
  simulation and agent-source driving — with identical resulting graphs.

**EDR-8 (observability).** Pull-observability (values snapshot, probes) and
purpose-built engine-graph probes; audio inspection emits
spectrogram/waveform + numeric features (agents can't hear).
- EDR-8.1: a probe mapping attached to any edge exposes its value stream
  over the values surface without altering region inference.

## Worked example (test seed)

An agent peer, using only advertised source nodes: opens the editor, walks
the store browser to hello-cosine, patches noise0 through a new vca, replaces
the auto latch with smoother (EDR-4.1), commits (ref moves), records a take,
marks its path through the store as a trace dataset — the medium's whole
gesture set exercised in one scripted, assertable run.
