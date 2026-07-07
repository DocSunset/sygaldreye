# Conformability executor: general lifting (rungs 3 + 5)

Task #59. Make lifting IMPLICIT and general per conformability.md (the
ratified rule: "match cells, lift frames, broadcast scalars, wrap like
mc audio"). Today lifting is hand-written everywhere (star_field,
particle_system, scatter→mesh_instances, audio channels). This item
replaces the hand-lifting with one executor rule.

## The one rule (from conformability.md)

A port's **natural cell is inferred from its kind** — scalar is a
1-cell, vec3 a 3-cell, mat4 a 16-cell, audio/span/mesh a whole payload.
NOT new declared metadata; the kind is the declaration. The executor
lifts on EXCESS rank only — wherever a payload's shape exceeds the
port's cell, along the excess axis. Frame-leading N×cell layout.
The axis IDENTITY (#58), not the count, picks the semantics:
- **map** (channel / item axis) = N INDEPENDENT STATEFUL INSTANCES,
  one state each — NOT one node re-run N times. Keyed by key field when
  keyed, by index otherwise; instance migration reuses the
  node-migration machinery.
- **scan** (time axis) = ONE instance threading state sample to sample.
Everything liftable (kernels, builtins, subgraphs = N inner-graph
clones) EXCEPT resource-holders (dac, http, editor, device streams) —
lifting a subgraph containing one is an error with a good message.

## DON'T REPEAT YOURSELF

The whole point is one rule, not N. There is ONE excess-rank check and
ONE strategy table. Strategies (CPU cell-map loop, N stateful clones,
audio channel scan, instanced draw) are each implemented ONCE and
selected by axis/statefulness/consumer-kind. No node hand-rolls a lift.
In particular there is ONE instanced-draw helper; the four nodes that
hand-roll instancing today must converge on it, not copy it.

## Rung 3 — stateful instancing across the graph (CPU)

The executor detects excess rank at an input. Stateless cell-map: loop
the pure kernel over rows, concatenate outputs to frames. Stateful map
axis: instantiate N instances (subgraph → N clone_graph copies; kernel
→ the per-channel kernel array `Lift` already holds), each with its own
state, migrated by the node-migration machinery. Audio is the existing
compile-time-stamped case (`Lift`); this rung adds the RUNTIME,
graph-scope cases (Span item/cell maps, subgraph clones).

## Rung 5 — GPU instancing strategy

An N×attribute array into a renderable IS GPU instancing — same rule,
the executor picks the instanced-draw strategy instead of a CPU loop.
The per-instance payload is NOT just a transform: it is whatever
columns the array carries (transform / position+size / position+color),
mapped to per-instance vertex attributes (divisor 1) by the Span's
named axes. mesh_instances is the manual per-row loop to retire first.

## Explicit verbs (build only as needed)

`each` is implicit (it IS lifting). Add explicit nodes only where output
shape depends on data or elements combine: **reduce** (mix is already
reduce(+)), **filter/compress**, **gather/scatter**. Cross-element
interaction (flocking/neighbors): kernel declares rank = whole-array.

## Acceptance: tree → forest, two routes (conformability.md)

1. N-array of seeds → lifted tree_generator → N distinct meshes
   (costs N generations).
2. One tree mesh + N×transform array at renderer → one instanced draw
   (cheap thousands).
The graph chooses by WHERE the array is wired. No new constructs.

## Acceptance: decompose the hand-rolled instancers into nodes + wiring

The four nodes that currently hand-roll their own instanced/batched
draw must be re-expressed as a base primitive + a per-instance attribute
array + the SHARED instanced-draw strategy — graph nodes and wiring, not
bespoke C++. Each one's hand-rolled GL loop is deleted, not refactored:
- mesh_instances  → one mesh + N×transform   (per-row loop today)
- particle_system → one quad + N×(pos,color,size) (own glDrawArraysInstanced)
- billboard_quad  → one quad + N×(pos,size), camera-facing (own instanced draw)
- star_field      → one point + N×position
- text_mesh       → glyph quad + N×(offset,uv) from a CPU layout node
Done when the strategy is implemented once and these consume it. Proves
"not more places to instance — stop reimplementing it."

## Status

- [x] RUNG 3 DONE (Part I, lifting-editor-drawfn branch). The executor
  lifts implicitly: ONE excess-rank check (span producer → cell-kind
  consumer, not whole) in build_plan → a LiftGroup; ONE strategy table in
  tick_graph (Clones / CellMap / resource-holder guard; Scan = the audio
  Lift, unchanged). Builtins AND subgraphs lift (subgraph = N
  desc->create() clone_graphs). State is keyed (lift_key cell value, else
  index) and survives reorder, resize, AND migrate_graph
  (Graph::lifted_store). Acceptance: forest route 1 — N seeds → lifted
  tree_generator → N distinct meshes (gathered to a MeshArray, drawn by
  forest_draw; host test + spectator screenshot). Resource-holder guard +
  subgraph inference verified. See planning/conformability_lifting.md
  rung 3.
- [ ] RUNG 5 (GPU instancing): scatter→color_mesh/sprite/wire_mesh
  positions stay wires (the instanced-draw boundary, span whole-by-kind —
  NOT lifted). The shared instanced-draw helper / retiring the four
  hand-rollers remains.

## Dependencies / sequencing

- Needs #58 (named-axis Span) and #57 (kernels as the authored form)
  first — lifting a kernel over a Span is the kernel template applied
  at graph scope.
- Enables retiring hand-lifting in star_field / particle_system /
  scatter, and unlocks vr_editor decomposition's cards-as-lifted-
  card-subgraph (vr_editor_decomposition.md slice 4).
- Deferred (designed-for, not built): rank > 2, tensor payloads,
  numpy-like matrix handles.
