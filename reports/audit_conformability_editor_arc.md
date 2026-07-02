# Audit: conformability lifting + editor decomposition arc (2026-07-01)

> **Remediation status (2026-07-02, this branch):** every finding below is
> resolved in code except the architecture-scale items, which have ratified
> backlog entries: card_subgraph_decomposition, edit_ops_over_wires,
> card_positions_as_graph_content, block_region_lifting,
> lifted_output_chaining, serial_id_escaping, cpp_size_overages,
> package_docs, werror_rollout, spectrogram_gl_migration, mesh_displace_gl,
> and the RT warm-pass note in audio_region_followups. Highlights: lift
> misuse is now a parse-time hard error (lint_lifts); slider drags apply
> in-place via queue_param (no rebuild); render caches are version-keyed
> (no stale geometry, no recompile storm, no per-eye double upload, texture
> eviction); ABI v9 generic context hook deletes peer_core's 13 type
> special-cases and reaches subgraph-nested nodes; edit ops are structural,
> escaped, and id-unique; ugen channel state survives count changes and
> follows the buffer sample rate; shells register at parity; the embedded
> editor graph is build-generated from assets/graphs/editor.json; rd_gpu /
> mesh_displace parked, spectrogram GL removed; dead GL components deleted;
> ADRs 006-011, kanban columns, and the five missing design docs + tests
> added. Full host suite: 59/59 test binaries green.

Scope: 87496a2 (squash of `lifting-editor-drawfn`, 46 commits), 4be6455,
04b0e69, 6d86ac9, 233e97a, 3a037bf, a099a98, 313b50e. Audited against
planning/vision.md, the ratified designs (conformability_executor.md,
vr_editor_decomposition.md, lifting_and_editor_decomposition.md), and
CLAUDE.md standards. Five independent review passes (lifting core, editor,
render, ugens, process); headline findings spot-verified in source.

## Verdict

The deletions are real and excellent: VrEditor/editor_node/vr_panel/
rgba_shader gone, DrawFn fully retired from built code, hand-rolled lift
plumbing gone from every processor ugen, one detection predicate + one
strategy table in the executor. But the arc's three headline claims are
each only **half-kept**, and the half not kept runs directly against the
guiding star (no-restart, editor-as-subgraph, live editing). In-headset
verification was waived, and the two worst defects are exactly the kind
that only shows up in-headset.

---

## 1. Vision violations (the "editor is just a subgraph" gap)

- **HIGH — peer_core.cpp:94-123: the meta seam regressed into ~13
  hardcoded type special-cases.** Plan S1 said replace the 2 special
  cases (`editor`/`spawner`) with a generic graph_source/edit_sink seam.
  Instead `pump_contexts` string-dispatches on ~13 node types and
  `static_cast`s each frame, top-level nodes only. A gesture node shipped
  as a plugin, or composed inside a subgraph, silently gets no context.
  Extending the editor = edit peer_core + both shells + recompile —
  the exact restart failure the guiding star names.
- **HIGH — `edit_sink` is dead on arrival.** No graph references it
  (zero hits across all JSON + embedded graphs); gesture nodes push
  straight into `edit_events_` via injected `GestureContext`. Edit ops
  never flow over wires, so they can't be observed/filtered/rewired
  in-graph. Registered, special-cased, tested, unused.
- **HIGH — `editor_layout` is the monolith's core transplanted as a C++
  library, not decomposed.** Card geometry, handle columns, slider
  layout, grid, palette placement are compile-time constants consumed by
  7 nodes holding raw `Graph*`. `card.json` is a 1-node wrapper around
  `card_mesh` — the plan's "card subgraph builds panel + per-port handles
  + sliders + label" and "palette rows = per-type lift" never happened.
  Restyling a card or moving the palette requires a rebuild
  (live_update_gap.md's exact complaint). graph_source.design.md's claim
  of being "the ONE node that reads the enclosing graph" is false ×7.
- **MED — card drag positions live in shell-owned state**
  (`PeerCore::editor_overrides_`, written directly by wire_drag.cpp:66).
  Not serialized, not undoable, invisible to remote peers/agents, lost on
  restart. graph_source.design.md says "updated by edit ops" — it is not.
- **MED — shell registration divergence.** Host registers the vec3/quat/
  mat/time/hsv/mesh-deform node families; app.cpp registers none of them.
  A host-authored graph fails to load on Quest — against "remote nodes
  just work" and the plan's own register-in-BOTH-shells discipline.
- **LOW — interim pumps key on node ids** (`n.id == "hand_r"` app.cpp:288,
  `"camera"` host_app.cpp:213). Rename the node, lose the hand. Tracked
  (xr_subsystem_nodes.md) and commented — acceptable as interim, fragile
  as shipped.

## 2. Correctness in the lifting core (signal_graph)

- **HIGH — UB on dangling edges: signal_graph_plan.cpp:148.**
  `has_other_excess` checks `f == idx.end()` but dereferences `t` without
  checking it. Dangling edges survive parse (`kind_of` → "unknown",
  `connection_legal("unknown","unknown")` is true). The editor graph
  always has a lift_key host, so every plan build scans every edge — one
  stale edge during live editing is a crash. Verified in source.
- **HIGH — lifted_store reattach never compares descriptors:
  signal_graph_tick.cpp:248.** `tick_clones` reuses the *stored* desc on
  key hit. migrate_graph's comment promises lazy reconciliation that
  doesn't exist. (a) Inline lifted subgraph across a swap → stored desc
  dies with old `owned_descriptors` → use-after-free. (b) Re-registering
  a subgraph type (live-reload card.json — the whole point of the
  project) leaves lifted instances on the OLD definition forever. Both
  untested.
- **HIGH — the resource-holder "hard error" is a stderr fprintf + fall
  through: signal_graph_plan.cpp:168-176.** The ratified design says hard
  error with a good message. Actual: warning, then the span→cell edge is
  classified as an ordinary v6 wire (connection_legal permits span→cell),
  binding a Span struct address as a float src. Graph loads, runs, reads
  garbage. Verified in source.
- **MED — a lifted host in the Block (audio) region never lifts.**
  Detection is region-agnostic but only `tick_graph` replays LiftGroups;
  `render_block` never consults them, and `wire_plan` has nulled the
  input. N-voices — the classic case — silently runs on defaults. Should
  be a plan-time error until audio lifting exists.
- **MED — keyed identity collides via `%g`: signal_graph_tick.cpp:174.**
  Keys format with 6 significant digits while graph_source emits 24-bit
  FNV hashes (8 digits) — truncation inflates collision odds ~100×; two
  cards then silently share one instance. One-character fix (`%.9g`);
  the hash-of-id layer itself is undocumented.
- **MED — silent garbage on the unsupported edges of a lift:**
  second excess-rank edge into a lifted host `continue`s into a raw
  span→cell wire (plan.cpp:157); edges from a lifted host's *non-first*
  output wire to never-ticked host storage; a gathered scalar output
  wired to a scalar consumer force-binds the gather Span as a float.
  None of these warn.
- **MED — claude_tmux is an unmarked resource-holder** (owns tmux
  sessions via `std::system`, has liftable scalar inputs). A span into
  `seq` → N clones each shelling out per tick. Every other holder was
  marked; this one was missed.
- **LOW** — lifted_store leaks entries when the lifted node/edge is
  deleted (cleanup only runs under live groups); per-tick rebuild of
  `host_group` map, key strings, and keep-set contradicts "lifting work
  belongs in build_plan/wire_plan"; zero-fill uses in-cell not out-cell
  stride (tick.cpp:191); `whole<T>` has zero production users
  (speculative, own audit said "NONE needed today").

## 3. Live-editing performance pathologies

- **HIGH — a slider drag rebuilds the entire graph every frame.**
  slider_drag emits a set_param op per tick while held; each op →
  serialize whole graph → string-patch → parse_graph (all nodes
  re-instantiated) → swap with pause_blocks + migrate_graph →
  `RenderRegion::notify_graph_swap` wipes ALL program/geometry caches →
  every shader recompiled + every static mesh re-uploaded, per frame, at
  72 Hz, on Adreno. The in-place `queue_param` path still exists
  (peer_core.cpp:73) — the editor just stopped using it. Same pathology
  class as the fixed 22-FPS wire bug, reintroduced; unobserved because
  in-headset verification was waived.
- **MED — every structural edit = full shader recompile + mesh re-upload**
  (maximally blunt cache wipe, even for instances migrate_graph just
  preserved). A generation tag or content-keyed cache fixes this plus the
  two staleness bugs below at once.
- **MED — per-frame CPU proportional to graph size × 7:** build_layout is
  recomputed independently by 7 editor nodes per tick, each running
  parse_port_schema per node AND `seed_slider_values` → `desc->serialize`
  (heap JSON dump) per slider node; undo_node serializes the whole graph
  every frame. ~200 node serializations/frame on a 30-node graph, idle.
- **MED — vertex_color_mesh marks everything dynamic** (hpp:82), and on
  Quest `issue()` runs per eye while `dyn_idx_` resets per frame → every
  dynamic mesh and instance Span uploads **twice per frame**. Terrain
  that changes only on param edits pays ~8 MB/frame of bus traffic.
- **MED — per-node-instance ShaderData defeats program dedupe:** N lifted
  cards = N compiles of the identical card shader (pointer-keyed cache),
  all recompiled on every edit per the wipe above.

## 4. "Structured ops" are relocated string surgery

signal_graph_edit.cpp admits it in its own header comment. The plan said
*replace* JSON splicing; it was moved and tested instead. Latent bugs:
- **HIGH — add_node ids are `type_<count+1>` with no uniqueness check:**
  `[osc_1, osc_2]`, delete osc_1, spawn osc → duplicate `osc_2`;
  duplicate ids then collide in the lift key column (two cards share one
  instance) and remove_node deletes only the first match.
- **MED** — remove_node matches `"id":"<id>"` anywhere in the serialized
  string (a param value containing it erases the wrong object); gesture
  nodes interpolate ids/ports into op JSON unescaped.

## 5. Render boundary: claims overstated

- **HIGH — "GL leaves every node" is false.** Three registered, unparked
  nodes still own GL: rd_gpu (registered in both shells while its only
  consumers are parked — a GL node with nothing to feed), MeshDisplace
  (inline glGenFramebuffers/glReadPixels, mesh_nodes.cpp:42), Spectrogram
  (own texture + glTexImage2D instead of the ImageTex path). None
  annotated or acknowledged.
- **HIGH — the draw-ordering mechanism landed unused.** DrawNode enqueues
  unconditionally (render_region_nodes.hpp:28 — contradicting the plan's
  "a draw node not wired into the head's chain never fires"), and no
  shipped graph but control_panel.json wires `seq` at all (editor.json: 7
  draws, 0 seq edges; sky/aurora have no render_head). Blended/no-depth
  draw order is therefore JSON-array accident; editor-driven reordering
  silently produces overlay artifacts. The "framebuffer is NOT
  commutative" rationale was the design's load-bearing argument.
- **MED — the stale-cache fix is partial.** notify_graph_swap misses
  `image_textures_` (pointer-keyed; safe only while all producers key on
  static atlases; also never glDeleteTextures — permanent leak), and the
  param-edit path (`queue_param`/deserialize, no swap) still serves stale
  geometry: dragging mesh_grid `cells` leaks a GlGeometry per regen and
  can render the old mesh on address reuse.
- **LOW** — instanced N taken from first attr span with no row cross-check
  (GL reads past a smaller VBO); the unlit MVP/vertex-color shader is
  pasted ~10×; MSDF text shader pasted 3×; the editor graph exists twice
  (embedded header + editor.json) and nothing loads the asset copy — plan
  S6 said the opposite; dead GL components still built (sphere_mesh,
  cube_geometry, billboard_quad's VAO code).

## 6. RT-audio (the Lift/Gen stamps)

- **HIGH — the Lift stamp codifies allocation inside the audio
  callback** (ugens.hpp:50-52): `kernels.assign` + `buf.resize` run in
  `render_block`. A live edit changing channel count makes DelayNode
  malloc+memset ~384 KB per channel in-callback → xrun. Pre-existing
  pattern, but the stamp was the one choke point to fix it once; instead
  it's stamped into every processor. kernels.hpp:11 promises "no
  allocation inside tick()" — honored per-sample, broken per-block.
- **MED — channel-count change wipes ALL kernel state** (assign on grow
  *and* shrink): 2ch→3ch resets surviving channels' delay tails/envelopes
  mid-note.
- **MED — delay/shaper/sample_hold/vca/slew have zero node-shell test
  coverage**, despite the plan's "A/B each against hand-written output".
  Near-miss on record: SampleHold's shared-clock hack silently depended
  on iteration order the planar flip changed.
- **LOW** — 48000 hardcoded through the shells while AudioBuffer carries
  sample_rate (44.1k device detunes silently); Mix/McPack still emit
  `audio` not `audio_out` (consumers probe both); osc wrap behavior
  changed inside a refactor whose acceptance was "output matching", with
  no waveform-pinning test; dac still declares `resource_holder` while
  3a037bf's doc says it doesn't own the stream — guard messages will
  state the disavowed claim.
- Net line count for the extraction arc: +47 (not the "code disappears"
  rhetoric) — the win is uniformity, which is real; block_shell itself is
  minimal and good.

## 7. Process / standards

- **HIGH — render_region, the arc's keystone (sole GL boundary), has no
  design.md and no test.** Same for render_payloads, sprite,
  vertex_color_mesh, color_mesh (5 of 20 new components). The other 15
  new components have genuinely complete design docs + substantive tests
  — that part was done well.
- **HIGH — adr.md is untouched (still ADR-001..005).** ABI v7/v8, DrawFn
  retirement, conformability lifting, render_region-as-boundary,
  editor-as-content: none recorded, despite CLAUDE.md's mandate.
- **HIGH — kanban was gitignored until the final commit** (313b50e dumped
  258 files); the workflow was unauditable for the whole arc, and 87496a2
  cites kanban files that didn't exist in-repo at that commit.
  `vr_editor_decomposition.md` (fully delivered, verification waived)
  still sits in `ready/`; `develop/` and `review/` directories don't
  exist at all; drawfn.md is a 0-byte item.
- **MED — 100-line rule breached without backlog items:**
  signal_graph_tick.cpp 267 substantive, signal_graph_plan.cpp 216,
  render_region.cpp 205, signal_graph_edit.cpp 147 (new). signal_graph —
  the core executor, heavily rewritten — has never had a design.md.
- **MED — zero package design docs exist**; CLAUDE.md's 4-package
  architecture (`scene: TBD`, `Goal: TODO`) no longer describes the app.
- **MED — synth_core.design.md contradicted by its own arc:** says "no
  dynamic allocation, no dependencies" while block_shell.hpp added
  std::vector + a public sygaldry_endpoints dep; not updated.
- **LOW** — status.md's top entry still claims ui_nodes/poke_* were
  deleted (the squash restored them); stale design-doc references to
  deleted editor_node in peer_core/host_app/hand_node/spawner_node docs;
  `id_key` FNV + default grid duplicated (graph_source vs editor_layout —
  drift makes hit-tests disagree with lift keys); no -Werror anywhere
  despite "treat warnings as errors".

## What to fix first

1. The three silent-garbage / UB holes in the lifting core (§2 H1-H3,
   M3/M4): make unsupported lift shapes and resource-holder lifts
   **plan-time errors**, and check both iterators. Small diffs, crash-
   and-corruption class.
2. The slider-drag rebuild storm (§3): route set_param through the
   existing `queue_param` in-place path. One-line class of fix; restores
   in-headset editability.
3. Descriptor check on lifted_store reattach (§2 H2) — it currently
   breaks live subgraph reload, the project's core promise.
4. `%g` → `%.9g` in row_key; uniqueness check in add_node id generation.
5. Generation-tagged render caches (fixes stale param-edit geometry, the
   texture leak, and the recompile-everything wipe together).
6. Paperwork: design.md + test for render_region; ADR entries; move the
   delivered kanban items; correct synth_core/graph_source design docs.
