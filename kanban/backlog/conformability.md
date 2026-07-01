# Conformability: rank-polymorphic lifting (RATIFIED 2026-06-12, Travis + Claude)

Replaces "replicator". There is no replicator node; there is one rule:

**Match cells, lift frames, broadcast scalars, wrap like mc audio.**

The multichannel audio system is the precedent and special case: a mono
biquad fed 2-channel audio lifts per-channel state; scalars broadcast;
counts wrap. Generalize those exact rules from AudioBuffer to all array
payloads. One rule system, not two.

## Rules

- Every input declares its **natural rank** — the cell it consumes:
  scalar, vec3, AudioBuffer, whole-array. Lifting happens on EXCESS
  rank only. A reverb whose inlet means "a whole buffer" does not lift
  over samples; a list-of-buffers lifts it.
- **Frame-leading layout, J-style: N×3**, not 3×N (matches interleaved
  point memory). A vec3 input fed an N×3 array sees cell (3), frame (N)
  → N lifted instances.
- **Everything is liftable — kernels, builtins, subgraphs** (subgraph
  lifting = N inner-graph clones) — EXCEPT nodes marked
  resource-holding (dac, http routes, editor, device streams).
  Lifting a subgraph containing one is an error with a good message.
- **State identity: by key field when the data is keyed, by index
  otherwise.** Lifted-instance migration shares the node-migration
  machinery (cards keyed by node id keep their state when the list
  reorders; audio channels lift by index, which is what channels mean).
- **Count is data, never a creation argument.** Pd clone's "wish I
  could resize it with a message" falls out for free.
- Outputs of lifted instances concatenate to frames (as mc audio does).
- Cross-element interaction (flocking, neighbor queries): declare the
  kernel's rank as whole-array. Not element-wise, not implicit.

## Explicit verbs (small, APL's core without the symbol zoo)

`each` is implicit (it IS the lifting). Explicit nodes only where
output shape depends on data or elements combine:
- **reduce** — fold a binop/subgraph; mix is already reduce(+)
- **filter/compress** — shape-changing selection
- **gather/scatter** — indexed rearrangement
(wiring is our composition; no other combinators needed)

## Lifting strategies (executor's per-region choice)

CPU loop, audio channels, **instanced draw** (an N×transform array into
the renderer's transform input IS GPU instancing), shader (texture
handles are already GPU-side array payloads). Same kernel, same rule,
different machinery — ties directly into kernel extraction (overnight
3) and the freezer.

## Acceptance test: tree → forest, two routes

1. N-array of seeds → lifted tree_generator → N distinct meshes
   (forest of individuals, costs N generations).
2. One tree mesh + N×transform array at the renderer → one instanced
   draw (forest of clones, cheap thousands).
The graph chooses by where you put the array. No new constructs.

## Dependencies / sequencing

Needs: span/array payloads (endpoints v6), declared ranks (endpoints
v6 metadata), kernel extraction (overnight 3). Enables: vr_editor
decomposition (cards = lifted card subgraph over graph_source.nodes,
keyed by id — see vr_editor_decomposition.md), procedural scenery
decomposition (star_field/particles/scattering are hand-written
lifting today).

Deferred (designed-for, not built): rank > 2, tensor payloads,
numpy-like matrix handles. Kernel signatures must not preclude them.

## Audio as named-axis matrix (ratified chat, 2026-06-12 afternoon)

- Per-sample kernels are the ONLY authored form; block processing is
  the executor's lifting loop, stamped PER KERNEL TYPE at compile time
  (make_descriptor-style template machinery) so generated shells match
  today's hand-written performance. Hand-maintained block ugens
  disappear; derived block-optimized kernels are freezer territory.
- Channels and time are DIFFERENT lifting semantics: channels = map
  (N independent kernel instances, per-channel state), time = scan
  (ONE stateful instance iterated, state threading sample to sample).
  "256 channels vs 256 samples" is disambiguated by AXIS IDENTITY, not
  count: audio is a 2-D array with NAMED axes (channels × frames),
  xarray/named-tensor style, not an anonymous matrix.
- Axis-rank rules: scalar kernel × (C×T) → C instances scanning T;
  FFT-like kernels declare rank over the time axis (consume it whole),
  still map over channels; spatialize CONSUMES the channel axis and
  produces a new one (axis-producing nodes exist).
- Representation: POD view + small named-shape header (generalizes
  AudioBuffer; rank ≤ 2 for now). NOT owning Eigen (allocation on the
  audio path, heavy dep for the freezer's embedded targets), NOT
  vector<vector>. Eigen::Map as a zero-copy opt-in ADAPTER only.
  Go PLANAR (channel-major) internally — lifted channel instances scan
  contiguous memory, SIMD-friendly, Pd-mc-style — interleave only at
  the engine/device boundary.

## Progress 2026-06-12 (afternoon)
- Span payload LANDED: rank-≤2 float view (rows × cols, frame-leading)
  in PortValue + ABI emit_span + /values ("span[NxM]"). v6 spans wire
  pointer-to-pointer; no legacy span producers exist by construction.
- Compat slots generalized to mesh (legacy mesh generators → v6
  consumers).
- First producer/consumer pair: scatter (N×3 positions) →
  mesh_instances (one mesh drawn per row — manual "lift the draw").
  FOREST ACCEPTANCE PASSES VISUALLY: 200 cylinders scattered on a
  ground plane, verified by screenshot through GET /screenshot.
- Named-shape header on Span + planar audio LANDED (task #58, 2026-06-14):
  Axis{Item,Cell,Channel,Time} on Span (survives the ABI), audio is
  channel-major internally with one interleave at the dac→device boundary.
- Still ahead: the conformability EXECUTOR (implicit lifting, axis
  rules — #59), true GPU instancing in mesh_instances.
