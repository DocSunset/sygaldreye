# Phase C design — the editor surface as graphs (EDR-1.2, EDR-7.2)

_Captured 2026-07-06 after the Phase-C understand pass (4-agent map of
lift/subgraph, graph_source, the value lane, glyph rendering, and the probe
editor). Phase A (pixels) and B (self-editing shell) are GREEN; this note is
the design the next session builds from. Goal: cards, wires, palette, labels,
gesture logic all live in `graphs/editor/*.json` over a small leaf
vocabulary — no editor-surface C++ (L22). Reuses the now-green render_region,
pointer, op_button, arbiter._

## What already works (reuse, don't rebuild)

- **render_region + draw chain** (PKG-4.2/4.3): render_head/frame → draw/tick
  → draw/chain, in wiring order; render_region reads each on-chain draw's
  held mesh+surface and issues GL. draw is passive.
- **pointer → op_button → arbiter self-edit** (PKG-4.4): a source-node event
  reaches op_button (poll delivers to sapply-or-apply) whose ops land in the
  graph's own arbiter. This IS the gesture→edit path.
- **lift** (rung 5): a producer with a `span`-kind out + an over-rank consumer
  → N clones `target#<key>`; a key span into the doc's `lift_key` port NAMES
  the clones by string id (survives reorder); a span into a span-kind port =
  whole-by-kind (sets `/n`, drops edge). ONE lift site per graph.
- **subgraph templates** (`graphs/*.json` as node types): inner nodes become
  `outer.inner`; a per-clone default `card#id/position` rewrites through the
  template's `inlets` to `card#id.pos/k` (creation-args — positions reach
  inner nodes WITHOUT wires, via set_num).
- **graph_source**: sees the ROOT doc by injection (`set_context`), publishes
  `keys` = node COUNT (a scalar). Reflection is injection-only (no globals).
- **mesh_from_spans / surface_flat / draw / mix / arithmetic leaves** exist.

## The hard design questions (resolve these FIRST, in order)

### Q1 — reflection drives a COMPILE-TIME lift, but the node list is RUNTIME

The lift reads a producer's **static** `values` default at compile time.
graph_source's node ids are **dynamic** (the graph is edited live). So
"graph_source → lift → one card per node" cannot work as-is: the lift can't
see a runtime output. **This is the crux of Phase C.** Options:

- **(A) Arbiter-synced card-source `values`.** On every structural edit, the
  editor also updates a `cardsrc/values` default = the current node-id list
  (and a positions list). Rebuild (which every structural edit already
  triggers) re-runs the lift over the fresh list → N cards. Mechanism: a
  node/graph that, on the target's edit, writes the node list into the
  card-source's `values` — OR the compile pass projects the target doc's
  nodes into the card-source. Cleanest candidate; needs a "project doc.nodes
  → a values span" step.
- **(B) Runtime reflector (rejected — L22 trap).** A C++ node that observes
  the graph each frame and emits N card meshes (the probe's forest_draw /
  card_widgets_mesh). This is exactly the editor-surface-C++ the greenfield
  forbids. Do NOT do this.
- **(C) Compile-time projection native.** A `graph_source`-like node whose
  `ids`/`positions` SPAN outs are materialized into the doc as `values`
  defaults by a compile/derivation pre-pass (the editor's compile reads the
  observed doc and seeds the card-source). Fits ADR-031 (derivation is the
  shape) — the editor's rendered view is a DERIVATION of the target doc.

**Recommendation:** pursue (A)/(C) hybrid — the editor's card layer is a
derived projection of the target doc's node list (ids + grid positions),
seeded into the card-source's `values`/keys at (re)compile, re-derived on
every structural edit (which already rebuilds). Prototype the smallest
version: a `syg edit`/shell op that, given a target graph, projects its
nodes into `cardsrc/values` + `cardsrc/keys`, compiles, and renders. Decide
whether the projection is a native pre-pass or an arbiter side-effect by
which keeps the lift the sole realizer (ADR-034).

### Q2 — gathering N card meshes into the frame: chain vs merge

Each lifted card produces a mesh (a quad at its position). To present them:

- **(chain)** N draws event-chained (render_head → card#0/draw/tick →
  card#0/chain → card#1/tick → …). But the lift rewires fan-in/out to the
  target's ports; it does NOT auto-serialize a chain among clones. Chaining
  lifted draws needs the lift to thread `chain→tick` through the clones — a
  lift capability that does not exist. Risky.
- **(merge)** Gather the N card meshes into a mesh span → a `mesh_merge`
  native (the mesh analogue of `mix`: concatenate vert lists) → ONE draw.
  lift.cpp REQUIRES a gathered clone-output to feed a span-kind consumer
  (else "excess rank" throws), so `mesh_merge` with a mesh-span input is the
  sanctioned shape. Needs: (a) a `mesh_merge` leaf (concatenate a span of
  mesh svalues), (b) the structured fan-in — how N clone mesh outs wire into
  one span-kind input (mix does this for audio via `/n`; confirm the
  structured analogue populates N sins or a span svalue). **Recommended** —
  matches the probe's forest_draw and one draw call is cheaper.

**Open:** does the current fan-in gather structured (mesh) outputs, or only
float/audio (mix)? Check build_impl's span fan-in wiring for structured
kinds; `mesh_merge` may need the gather widened to the structured lane.

### Q3 — labels: glyph_quads + the font atlas as a dataset

- **glyph_quads leaf** (the ONLY new leaf Phase C strictly needs): a `text`
  default (set_text, like mesh_from_spans/positions) → a mesh whose verts are
  **x,y,u,v (4/vert)**. Port `glyph::layout` from
  `probe/components/text_mesh/glyph_layout.hpp` (per-glyph: advance,
  planeBounds→quad, atlasBounds→uv; V FLIPPED; space advances without a quad).
- **carriers must widen**: `mesh_data` is 2-float xy — a glyph mesh is 4-float
  xyuv, so the vertex layout must be program-aware (render_region keys the
  attrib layout off `surface.program`). `surface_data` needs an `atlas_cid`
  (reference-by-CID, NOT inline pixels — honesty) and a `program="glyph"`.
- **render_region**: compile a second `glyph` MSDF program (median-of-3 +
  screen-px-range; shader verbatim in the map), a SEPARATE atlas texture
  (GL_RGB8 — the 3 channels are signed distance, not colour), uRange from
  distanceRange=2, blend on. Resolve atlas CID→bytes→stbi_decode→upload once,
  cache by CID.
- **atlas dataset (honest)**: `chunk-put` the 43 KB `probe/assets/fonts/
  dejavu_sans.png` once, reference by CID. Metrics: bake the tiny 95-row
  table for v1 (auditable), metrics-as-dataset a later succession. Do NOT
  bake the PNG (the probe's dishonest route).

### Q4 — wired/computed positions (the dual-hook) — OPTIONAL for v1

Structured constructors (mesh_from_spans) read dx/dy via set_num only;
svalue_tick gets no float ins, so a WIRED position doesn't reach the mesh.
BUT: card positions flow as per-clone lift **defaults** (set_num) and drag
edits go through **set_param** (set_num) — both work today. So the dual-hook
is NOT required for v1. It IS the clean fix when geometry must be COMPUTED
through wired edges (a grid from an index span, an LFO-driven position). The
fix (confirmed viable — build_impl already populates a structured node's
float `ins`, exec_plan.cpp:304): change the frame-tick dispatch from
`if svalue_tick … else if value_tick` to run value_tick THEN svalue_tick
(dedupe the ++recomputed / last_tick_order / cone-dirty bookkeeping to once
per node), and add a value_tick to mesh_from_spans that loads in_vals[dx/dy]
into state. Land it when a wired position is first genuinely needed.

## The build order (once Q1–Q2 are decided)

1. **Leaves/capabilities:** `mesh_merge` (Q2), the reflection projection (Q1),
   `glyph_quads` + atlas dataset + glyph program (Q3). Each with a focused
   test before any graph.
2. **graphs/editor/card.json** — one card: a `mesh_from_spans` body quad at a
   `position` inlet + `surface_flat` fill + `glyph_quads` label. lift_key=id.
   (Probe card.json is minimal — body only; widgets/labels can be graph-wide
   meshes reading a shared layout. Decide per-card vs graph-wide.)
3. **graphs/editor/cards.json** — the reflection → keyed card lift → merge →
   draw. First VISIBLE milestone: N quads at grid positions for an observed
   graph's N nodes. (This is EDR-1.2's heart — send a PNG for blessing.)
4. **graphs/editor/wires.json** — edges → line-strip meshes (mesh_from_spans
   with 2-vert line geometry, or a line primitive), merged, drawn.
5. **graphs/editor/palette.json** — registry-face names → rows of label +
   spawn zone (extend palette_osc/palette_noise, already EDR-1.1 fixtures).
6. **graphs/editor/gestures.json** — pointer → gesture FSMs → op_button →
   arbiter. Preserve the probe's hard-won invariants (below).
7. **graphs/editor/editor.json** — the composition root; default for `syg shell`.
8. **Witnesses:** EDR-1.2 (`syg palette` == packages.json + a gates.sh grep:
   no card/palette/wire_drag/slider_drag/dwell/gesture in src/nodes; every
   graphs/editor/*.json round-trips) and EDR-7.2 (from an EMPTY graph, a
   scripted pointer session authors hello-cosine; topology matches; renders +
   passes golden-audio).

## Layout constants + gesture invariants to honor (probe, hard-won)

- **Layout (metres, world-space):** card spacing 0.45, card W 0.4, base H
  0.08, **port row pitch 0.018**, handle half 0.006 / off-edge 0.01, slider
  width 0.6·cardW, controller tip = pose·(0,0,−0.05). Card height =
  base + rowPitch·(wirable inputs). Eye-level reachable band (y 1.85→0.35,
  z −0.5), grid wraps by block. (Host/2D adapts these; XR uses them literally.)
- **Slider range from the port KIND schema** (define-once), value seeded from
  live serialized params — never per-node literals.
- **Gestures (each = pointer pos/rot + ONE activation channel + shared
  overrides cell + op_button):**
  - pick = nearest-handle within radius (feedback only, never edits).
  - wire_drag (grip): rising → grab nearest OUTPUT (0.02 m) else MOVE card;
    falling → nearest kind-COMPATIBLE INPUT (0.03 m, connection_legal) →
    add_edge. Nearest-on-DROP, not on-press. One grip FSM, two outcomes.
  - slider_drag (trigger held): one y-nearest slider; tip-x → set_param.
  - dwell_delete (grip HELD + 1.0 s dwell): remove_node/edge; card wins over
    edge. **Bare hover NEVER deletes** — the key safety fix.
  - palette (trigger rising edge = poke): row → add_node(types[idx]); header
    row pages (wraps); out-of-range inert.
  - undo = left thumb flick (separate channel).
- **The shared overrides cell** (per-id card position) is read by BOTH the
  move gesture and the card render, so a dragged card renders where it's
  dragged. Load-bearing.

## Salvage (reference only — RE-EXPRESS as graphs, never migrate)

`probe/assets/graphs/editor.json` + `card.json` (the composition + minimal
card ancestors); `probe/components/text_mesh/glyph_layout.hpp` +
`text_label/` (glyph algorithm + MSDF shaders); `probe/assets/fonts/
dejavu_sans.{png,json}` (atlas — chunk-put it); `editor_layout` constants
above. The probe is DEPRECATED; these are ancestors, not code to port.

## Discipline (unchanged)

Test + HARNESS row before implementation; commit per criterion with the id;
fresh-context audit before opening the next rung; status.md resume block at
every stop. The whole phase is L22 with a schedule: if a surface behavior
can't be a graph yet, the missing leaf/capability is the bug — build the
leaf, then author the graph.
