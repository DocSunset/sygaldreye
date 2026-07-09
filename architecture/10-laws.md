# Chapter 10 — The Law

*The authoritative gathering of the principles, laws, and protocols the
project must achieve, the requirements it must satisfy, and the association
between the two. When a design choice and this chapter disagree, this chapter
wins or must itself be amended — recordedly.*

## I. Fundamental needs

Derived from `planning/vision.md` and the dream it serves.

- **need.liveness.** Any change — feature, bugfix, extension, at any level
  including the compiler — lands without restarting. The guiding star: a
  required restart is a bug, upstream.
- **need.uniformity.** Everything is expressible as nodes, graphs, and
  datasets — content, editor, compiler, infrastructure — over a finely
  curated, minimal vocabulary; composition precedes new primitives.
- **need.mesh.** Multiple devices (host, headset, browser) are one
  environment: remote node types just work; placement follows capability.
- **need.agency.** LLM agents are peers, not backdoors: same inputs, same
  editor, same rules; agents can extend the running system (codegen loop)
  under law.
- **need.memory.** What matters is durable with honest provenance: takes,
  versions, undo, re-derivability; nothing silently lost, nothing silently
  overwritten.
- **need.practicality.** Stands on existing compilers and toolchains;
  hard-real-time audio and frame-rate XR are met; no vanguard type theory in
  the runtime.
- **need.medium.** The same data model carries documents and media
  (the rhizome layer) and remains honest at web scale.
- **need.safety.** The mesh is a security boundary: running other people's
  graphs is safe by construction; capability escalation is deliberate and
  attributable.

## II. Laws

**Ontology**

- **law.one_form.** Data is the only form — bytes, meaning conferred only by
  decoding. Node (container) and link (reference) are its two primal
  functions; every other concept is a role. Never reify a function as a form;
  form affords function.
- **law.mutability_in_the_name.** Hashes name immutable values; routes
  name editable places; an address is live if and only if it traverses a ref. No other
  mutability mechanism exists.
- **law.declarations_one_level_up.** Promises and schemas are links *about* link
  names held by the node type; instances hold the links. Checked at
  edit/commit time by a first-order oracle; never consulted at tick time.

**Data**

- **law.provenance_or_fork.** Every derived thing tracks its sources or has
  recordedly detached. Never silent.
- **law.two_doors.** Data becomes durable only by committed derivation (recipe) or
  capture (testimony). Streams cannot promise immutability; commitment is an
  act.
- **law.ground.** Every regress ends in declared ground: sacred kinds,
  captures, natives, stage 0, the header convention — fiat and testimony,
  recorded as such. Below stage 0, provenance continues as build derivations.
- **law.format_is_law.** The at-rest form (bags of links, datasets) is the
  contract; every runtime representation is an observationally equivalent
  cache that must round-trip.

**Execution**

- **law.derive_dont_mutate.** Ticking is uncommitted derivation;
  instantiation is uncommitted derivation in space; commit turns either into
  a dataset. Regions are inferred, never declared; boundaries are reified as
  visible, replaceable mappings.
- **law.executors_own_pacing.** Every region's cadence and platform resource
  belong to its executor node; no node reaches around its region's contract.
  Real-time regions allocate nothing after init; resources acquire on first
  tick, never in create().
- **law.existence_is_reference.** Instances exist while linked; datasets
  exist while provided. Creation and destruction are link operations plus
  collection; there is no other lifecycle.

**Compilation**

- **law.roles_not_types.** App/engine/execution are roles in a
  compilation relationship; every graph is defined, compiled, realized —
  including engine graphs. The tower is lazy and grounds at the frozen
  stage 0, whose sole irreducible property is pre-existence.
- **law.identity_preserving_compilation.** Compilation is a deterministic derivation, committed and hence memoizable, emitting a route to route map; state survives
  re-compilation through it.
- **law.one_definition.** Realized views are editing surfaces writing back
  through the inverse map; defaults insert only where absent; conditional
  behavior is a deliberate pass; refusal is a fork. No shadow definitions,
  ever.
- **law.ports_and_wiring.** Engine extension points are published ports;
  packages wire in; order is wiring, visible as structure.

**Greenfield (ADR-013 through 028)**

- **law.the_ratchet.** The core only shrinks: admission requires a
  no-composition proof (ch. 16); presence is always a tape choice; nothing
  is imposed, all the way down.
- **law.errors_are_values.** Noexcept by default;
  declared fault outputs; promise-breaking kills the containment unit;
  supervision policy is wiring; detection is demand-driven overlay.
- **law.succession.** Nodes are succeeded, never mutated; migrations are lazy, committed derivations; compatibility is reachability; nothing is ever forced
  to rewrite.
- **law.suite_is_the_system.** Conformance defines sygaldreye; the
  suite is written in the system; blessing is testimony; the candidate is
  a peer.
- **law.graphs_first.** The default form of every behavior is a graph;
  a native requires a declared clause (ADR-033); anything called a graph
  is realized through the one contract (ADR-034). When something cannot
  be authored as a graph, the missing vocabulary is the bug — build the
  nodes that make it authorable, then author it. (The second guiding
  star, ch. 1.)

**Mesh**

- **law.advertisement_is_permission.** Placement is pull-shaped; a peer
  runs, serves, and subscribes to exactly what it publishes. Selective
  advertisement is the sandbox; only vocabulary and refs are ever gated,
  never the data model.
- **law.graphs_flow_plugins_gated.** In-mesh graphs realize freely
  within advertisement; native code is a distinct, provenance-gated,
  signed act.
- **law.keys_not_secrets.** Identity is per-peer keypairs; testimony and
  plugins are signature-verifiable; membership is revocable; secrecy, when
  built, is decoder scarcity.

## III. Protocols

The system, in one sentence (vision.md): *a protocol for encoding recursive
metadata and decoding data to derive other data — grounded in stage 0, paced
by executors, shared by a mesh, terminating in someone's senses.* Its
concrete protocol surfaces, each governed by the laws cited:

- **Naming and resolution** (law.mutability_in_the_name, law.declarations_one_level_up, law.format_is_law): CID/multihash; routes-from-here addresses (root:route as sugar);
  traversal per kind; normalization/memoization of fixed addresses.
- **Commit and provenance** (law.provenance_or_fork, law.two_doors): recipe and testimony headers; hashes
  only in provenance; memoization keyed by input hashes.
- **Store and fetch** (law.format_is_law, law.existence_is_reference, law.advertisement_is_permission): store-as-graph access; provide/compatible
  advertisement; content-addressed, chunked, resumable fetch; back-link
  indexing at commit.
- **Compile and realize** (law.roles_not_types–law.ports_and_wiring): engine fan-ins; compilation maps;
  swap+migrate keyed by route and lift key.
- **Mesh session** (law.advertisement_is_permission–law.keys_not_secrets): pairing, authenticated transport, three-list
  advertisement, typed refusal, plugin gate.
- **Boot** (law.ground, law.roles_not_types): trampoline to frozen stage 0 to spawn-and-park to observe,
  splice, receive, compile, realize.

## IV. Traceability

Requirements live in chapters 2–9 (`NAM, STO, EXE, CMP, SZ, PKG, MSH, EDR`).
Association, requirement to laws to needs:

| Requirement | Laws | Needs |
|---|---|---|
| nam.addresses, nam.liveness | law.mutability_in_the_name | need.uniformity, need.memory, need.mesh |
| nam.edit_stable_routes | law.mutability_in_the_name, law.identity_preserving_compilation | need.liveness, need.memory |
| nam.one_kind_system | law.one_form, law.declarations_one_level_up | need.uniformity, need.medium |
| nam.promise_oracle | law.declarations_one_level_up, law.derive_dont_mutate | need.uniformity, need.practicality |
| nam.hash_format | law.format_is_law | need.memory, need.medium, need.mesh |
| nam.sequence_traversal | law.mutability_in_the_name, law.format_is_law | need.medium, need.memory |
| sto.object_machinery, sto.fetch | law.format_is_law | need.memory, need.mesh |
| sto.store_graph_face | law.one_form, law.advertisement_is_permission | need.uniformity, need.safety |
| sto.commit_paths | law.provenance_or_fork, law.two_doors | need.memory |
| sto.refs_and_undo | law.mutability_in_the_name, law.provenance_or_fork | need.memory, need.liveness |
| sto.provide_compatible | law.existence_is_reference, law.advertisement_is_permission | need.memory, need.mesh |
| sto.composite_graphs | law.one_form, law.one_definition | need.uniformity, need.memory |
| sto.retention_and_forgetting | law.provenance_or_fork, law.existence_is_reference | need.memory, need.safety |
| sto.back_link_index | law.provenance_or_fork | need.memory, need.medium |
| exe.plan_cache | law.format_is_law | need.practicality, need.uniformity |
| exe.region_inference | law.derive_dont_mutate | need.uniformity, need.practicality |
| exe.realtime_safety | law.executors_own_pacing | need.practicality |
| exe.canonical_mappings | law.derive_dont_mutate | need.practicality, need.uniformity |
| exe.migration | law.identity_preserving_compilation | need.liveness |
| exe.executor_contract | law.executors_own_pacing | need.uniformity, need.practicality |
| exe.derivation_mode | law.two_doors, law.derive_dont_mutate | need.memory, need.uniformity |
| exe.visible_boundaries | law.derive_dont_mutate, law.one_definition | need.liveness, need.uniformity |
| exe.existence_is_reference | law.existence_is_reference | need.uniformity |
| cmp.compile_is_derivation | law.provenance_or_fork, law.identity_preserving_compilation | need.memory, need.practicality |
| cmp.determinism_and_map | law.identity_preserving_compilation | need.liveness |
| cmp.extension_ports | law.ports_and_wiring | need.uniformity |
| cmp.projection_editing | law.one_definition | need.liveness, need.uniformity |
| cmp.fork | law.provenance_or_fork, law.one_definition | need.memory |
| cmp.laziness | law.roles_not_types | need.practicality |
| cmp.identity_across_recompilation | law.identity_preserving_compilation | need.liveness |
| cmp.retired | law.format_is_law, law.roles_not_types | need.practicality |
| cmp.engine_is_realized | law.roles_not_types, law.ports_and_wiring, law.graphs_first | need.liveness, need.uniformity |
| sz.platform_invariance, sz.trampolines | law.roles_not_types | need.uniformity, need.practicality |
| sz.generated_registration | law.ground, law.advertisement_is_permission | need.uniformity, need.safety |
| sz.naive_resolver | law.ground | need.practicality, need.memory |
| sz.frozen_with_provenance | law.provenance_or_fork, law.ground | need.memory, need.practicality |
| sz.spawn_and_park | law.roles_not_types | need.liveness, need.practicality |
| sz.boot_without_store | law.ground | need.practicality, need.mesh |
| pkg.package_shape | law.ports_and_wiring | need.uniformity |
| pkg.audio_retrofit | law.format_is_law, law.executors_own_pacing | need.practicality |
| pkg.xr_package, pkg.render_package | law.derive_dont_mutate, law.executors_own_pacing | need.liveness, need.uniformity |
| pkg.worker_derivation | law.two_doors, law.derive_dont_mutate | need.mesh, need.memory |
| pkg.net_package | law.derive_dont_mutate, law.advertisement_is_permission | need.mesh |
| pkg.placement_as_fallthrough | law.identity_preserving_compilation, law.ports_and_wiring | need.mesh, need.uniformity |
| pkg.environment_observation | law.ports_and_wiring | need.liveness, need.mesh |
| msh.keypairs_pairing, msh.authenticated_transport | law.keys_not_secrets | need.safety, need.mesh |
| msh.three_lists, msh.pull_shaped_placement | law.advertisement_is_permission | need.safety, need.mesh |
| msh.graphs_vs_plugins | law.graphs_flow_plugins_gated | need.safety, need.agency |
| msh.capture_testimony_keys | law.keys_not_secrets, law.two_doors | need.safety, need.memory |
| msh.discovery | law.advertisement_is_permission | need.mesh, need.medium |
| msh.graded_circles | law.advertisement_is_permission, law.keys_not_secrets | need.safety, need.medium |
| exe.per_sample_islands | law.format_is_law, law.derive_dont_mutate | need.practicality, need.uniformity |
| exe.quiescence_and_demand | law.derive_dont_mutate, law.executors_own_pacing | need.practicality, need.uniformity |
| frz.round_trip | law.provenance_or_fork, law.format_is_law | need.memory, need.practicality |
| frz.tier_computation | law.provenance_or_fork | need.practicality, need.uniformity |
| frz.freestanding_proof | law.format_is_law | need.practicality |
| frz.service | law.advertisement_is_permission, law.graphs_flow_plugins_gated | need.mesh, need.safety |
| lng.kind_catalog, lng.span_semantics | law.one_form, law.declarations_one_level_up | need.uniformity, need.practicality |
| lng.event_discipline | law.derive_dont_mutate | need.practicality, need.uniformity |
| lng.inlet_model | law.one_form, law.one_definition | need.uniformity, need.memory |
| lng.edit_ops | law.derive_dont_mutate, law.one_definition | need.liveness, need.agency, need.memory |
| lng.subgraphs | law.one_form, law.existence_is_reference | need.uniformity |
| lng.context_seam | law.executors_own_pacing | need.uniformity, need.practicality |
| lng.round_trip | law.format_is_law | need.memory, need.practicality |
| lng.text_events | law.derive_dont_mutate | need.liveness (open) |
| lng.query_vocabulary | law.one_form, law.derive_dont_mutate | need.uniformity, need.memory, need.medium |
| lng.structured_payloads | law.one_form, law.declarations_one_level_up, law.format_is_law, law.graphs_first | need.liveness, need.uniformity, need.agency |
| aut.kernel_contract, aut.shell_stamps | law.format_is_law, law.executors_own_pacing | need.practicality |
| aut.generated_descriptors | law.declarations_one_level_up, law.format_is_law | need.uniformity, need.practicality |
| aut.lift_guarantees | law.identity_preserving_compilation | need.liveness, need.uniformity |
| aut.four_routes | law.one_form, law.graphs_flow_plugins_gated | need.uniformity, need.agency |
| edr.editor_is_nodes | law.one_form, law.derive_dont_mutate | need.uniformity, need.liveness |
| edr.defaults_discipline, edr.undo | law.mutability_in_the_name, law.one_definition | need.memory, need.liveness |
| edr.realized_view_editing | law.one_definition | need.liveness |
| edr.store_browser | law.one_form, law.derive_dont_mutate | need.medium, need.uniformity |
| edr.documents | law.mutability_in_the_name, law.format_is_law | need.medium |
| edr.agents_as_peers | law.advertisement_is_permission | need.agency, need.safety |
| edr.observability | law.derive_dont_mutate | need.agency, need.practicality |
| abi.one_declaration, abi.hook_discipline, abi.fault_declaration, abi.contract_succession, abi.three_packagings | law.declarations_one_level_up, law.format_is_law, law.errors_are_values, law.succession | need.uniformity, need.practicality, need.safety |
| fmt.encoder_conformance, fmt.address_grammar, fmt.boot_tape, fmt.wire_transcripts, fmt.pins_frozen | law.mutability_in_the_name, law.format_is_law | need.mesh, need.memory, need.practicality |
| tcf.mapping_guarantees, tcf.swap_safety, tcf.clock_honesty, tcf.fault_matrix, tcf.movement_austerity | law.derive_dont_mutate, law.executors_own_pacing, law.errors_are_values | need.practicality, need.liveness |
| cor.escapement_austerity, cor.crown_sufficiency, cor.ratchet_enforcement, cor.two_profiles | law.ground, law.roles_not_types, law.the_ratchet | need.uniformity, need.practicality |
| cor.native_ledger_discipline | law.graphs_first | need.uniformity, need.practicality |
| cnf.suite_as_data, cnf.candidate_as_peer, cnf.two_profiles, cnf.succession_end_to_end, cnf.self_gate | law.format_is_law, law.succession, law.suite_is_the_system | need.memory, need.practicality, need.uniformity |
| cnf.derived_versions | law.succession, law.suite_is_the_system | need.memory, need.uniformity |

*(New-chapter requirements are traced at chapter granularity; per-item rows
join as the conformance suite (cnf.suite_as_data) materializes them as test datasets.)*

Reading the table backward answers "why does this requirement exist": every
row terminates in the needs, and every need traces to the vision. A proposed
requirement that maps to no law and no need should be rejected; a law that no
requirement exercises is untested doctrine and needs one.

## V. The payoff — what the finished system gives you

The needs and laws above are abstract; this is what they buy, concretely.
Every item is a consequence of the machinery in chapters 2–9, cited.

**Playing and making**

- Patch a VR world and a modular synth *while they run* — scene, sound,
  camera, and interaction rewire live, mid-performance, no restart, state
  carried across every edit (exe.migration, cmp.identity_across_recompilation).
- Every boundary the system inserts — latches, queues, network hops — is a
  visible node on the canvas you can inspect and replace, where other
  environments hide them (exe.visible_boundaries). This is where we beat Max.
- Feedback, cross-rate wiring, and cross-device wiring are all just wires;
  the compiler picks the right adapter and shows its work (exe.canonical_mappings, pkg.placement_as_fallthrough).
- Presets are datasets: swap a sound's whole parameter face without touching
  the patch; diff two presets like code (sto.composite_graphs).

**The mesh as one instrument**

- A node type living on another machine "just works" — controllers on the
  headset driving synthesis on the desktop, visuals in a browser tab
  (pkg.net_package).
- Capability placement: open a heavy patch on a weak device and the heavy
  regions compile onto peers that can carry them; a browser peer becomes a
  spectator of the headset's world automatically (pkg.placement_as_fallthrough).
- Hot-plug a device and its whole vocabulary splices into the running
  compiler; unplug it and placement falls through to the mesh (pkg.environment_observation).
- The whole session is supervised: crash a level and its parent restarts it;
  stage 0 itself can never be edited away (sz.spawn_and_park).

**Memory that behaves like memory**

- Infinite, branching undo across sessions — history is hash trails, so
  nothing is ever silently overwritten, and any version is one rebind away
  (sto.refs_and_undo).
- Record a take anywhere and it carries testimony: which peer, what wiring,
  signed (msh.capture_testimony_keys). Derived artifacts carry recipes and rebuild themselves —
  re-derivation replaces backups for everything except the irreplaceable
  (sto.commit_paths, sto.provide_compatible).
- Deliberate forgetting is supported: unpin everywhere and the mesh
  converges — deletion is a decision, not an accident (sto.retention_and_forgetting).
- Identical data stores once, mesh-wide, verified by hash (nam.hash_format).

**The compiler is yours**

- The thing that turns patches into running systems is itself a patch: open
  it, probe it, extend it by wiring into its published ports; its pass order
  is readable off the canvas (cmp.extension_ports, law.ports_and_wiring).
- Compilation is memoized — editing a parameter recompiles nothing
  structural (cmp.compile_is_derivation).
- Freeze any patch to a standalone C++ program for the smallest targets, and
  unfreeze it later — the artifact carries its own source (law.format_is_law, sz.frozen_with_provenance).
- Offline mode: render two seconds — or two hours — of any patch as a
  provenance-carrying derivation on the worker region; the build system is
  just another region (exe.derivation_mode, pkg.worker_derivation).

**Agents in the band**

- An LLM agent is a peer: it flies the camera, operates the editor, and
  reproduces your bugs through the same source nodes as your hands (edr.agents_as_peers).
- The codegen loop: an agent writes a new node in C++, a worker derivation
  cross-compiles it, the artifact ships by hash with a signed provenance
  chain, and the headset hot-loads it mid-set (msh.graphs_vs_plugins, ch. 8 example).
- Agents can't hear or see the room, so the system shows them: probes,
  values, screenshots, spectrograms on any edge, without disturbing the
  patch (edr.observability).

**Sound design without asterisks**

- Feedback is one sample by default: Karplus-Strong, feedback FM, and
  filters-in-loops just work, live and interpreted; freezing only makes them
  cheap (exe.per_sample_islands, frz.round_trip). Explicit delays opt out — the choice is wiring.
- Polyphony is a channel count: wire a span of frequencies into one osc and
  get N keyed voices, no voice-manager node, identity preserved through
  resize (lng.span_semantics, aut.lift_guarantees).
- Freeze any patch to a portable C++ class — a plugin, a firmware image, an
  embedded instrument authored in VR — and unfreeze it back to the editable
  graph; a microcontroller running one can stay a peer, advertising its
  nodes to the mesh (frz.round_trip–4).

**Horizons (validation targets, not requirements — the kind system must not
preclude them)**

- An additive synthesizer whose partials are a dataset driving an oscillator
  pipeline; atomic-decomposition analysis-resynthesis; a video
  editor-compositor; projection mapping; an ML workbench where a model
  carries provenance to its training set (ADR-006's five applications).
- The companion and spectator apps as *just more subgraphs* — expressed
  through, and hackable via, the same graph model (vision.md).
- The brainstormed application space (`kanban/applications/`, 2026-06),
  whose uniting thread is **closing the research–application gap**: a paper,
  model, dataset, or algorithm authored, explored, and shipped as an
  interactive thing in one medium — research artifact and app are the same
  patch. The notes, each riding machinery this book specifies:
  *musical instruments* (VR lutherie on the audio substrate; EXE, ADR-013),
  *game engine* and *physics sandbox* (the patch is the game / the physics
  is the content), *CAD, sculpting + slicer* and *3D scanner + LLM model
  viewer* (scan to sculpt to print; geometry as wirable content; agents
  seeing models via edr.observability-style rendered views), *robotics firmware, 3D
  printers* and *robot gardening* (one medium from motor loops to behavior —
  frozen peers, FRZ, are the firmware story), *home automation* (devices
  and sensors as edges; the mesh reaching the house), *ML authoring
  environment* ("Wekinator 2026": hand-author interpretable layers, tweak
  live, train as derivations), *code/document explorer + hypertext editor*
  and *web UI/hypertext content* (the rhizome layer's reading and authoring
  faces; edr.documents), *social appliverse* (patches as places people inhabit
  together — msh.graded_circles's graded circles grown up).

**A medium, not just an app**

- Documents are graphs: transclude anything — a parameter, a take, a
  paragraph — as quotation (fixed) or live subscription; links are
  bidirectional and survive editing (edr.documents, nam.sequence_traversal).
- Walk everything: the store browser's ground/frontier/mark turns the whole
  mesh — patches, takes, kinds, provenance — into one navigable space, and
  your walked trace is itself a committable, shareable dataset (edr.store_browser).
- A codebase can be decomposed into the medium and regenerated byte-identical
  — every open-source repo is a unit test for the document layer (edr.documents.cpp_roundtrip).
- The trust model reads like a patch: a peer's sandbox is literally the list
  of node names it advertises; a package is a provenance closure you can
  re-derive instead of trust (msh.three_lists, law.advertisement_is_permission–law.graphs_flow_plugins_gated).

## VI. Amendment

This chapter changes only by ratified decision, recorded in `adr.md` with the
reasoning, and reflected here in the same commit. Terminology is governed by
`planning/lexicon.md` (one name per concept, one concept per name; form vs
function; the retired-prose list is binding on all documents).
